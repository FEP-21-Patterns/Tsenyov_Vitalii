#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <algorithm>
#include <stdexcept> // Для std::runtime_error
#include <cctype>    // Для isdigit

// ============================================================================
// HIERARCHY OF DATA TYPES (STRATEGY PATTERN)
// ============================================================================

class DataType {
public:
    virtual bool validate(const std::string& value) const = 0;
    virtual std::string getName() const = 0;
    virtual ~DataType() = default;
};

class IntegerType : public DataType {
public:
    bool validate(const std::string& value) const override {
        if (value.empty()) return false;
        try {
            size_t pos;
            std::stoi(value, &pos);
            // Переконуємося, що *весь* рядок є числом, а не лише його початок (напр., "123xyz")
            return pos == value.size();
        } catch (const std::exception&) {
            return false;
        }
    }
    std::string getName() const override { return "Integer"; }
};

class StringType : public DataType {
public:
    bool validate(const std::string& value) const override { return true; }
    std::string getName() const override { return "String"; }
};

class BooleanType : public DataType {
public:
    bool validate(const std::string& value) const override {
        return value == "true" || value == "false" || value == "1" || value == "0";
    }
    std::string getName() const override { return "Boolean"; }
};

class DateType : public DataType {
public:
    bool validate(const std::string& value) const override {
        // Покращена базова перевірка формату YYYY-MM-DD.
        // Справжня реалізація мала б розбирати дату та перевіряти її коректність (напр., місяць <= 12).
        if (value.size() != 10) return false;
        if (value[4] != '-' || value[7] != '-') return false;
        
        for(int i : {0,1,2,3, 5,6, 8,9}) {
            if (!isdigit(value[i])) return false;
        }
        return true;
    }
    std::string getName() const override { return "Date"; }
};

// ============================================================================
// DATABASE CORE CLASSES
// ============================================================================

class Column {
private:
    std::string name;
    std::shared_ptr<DataType> type;
    bool nullable;
    bool primaryKey;
    std::optional<std::pair<std::string, std::string>> foreignKey;
public:
    Column(const std::string& n, std::shared_ptr<DataType> t, bool nullb = true, bool pk = false,
           std::optional<std::pair<std::string, std::string>> fk = std::nullopt)
        : name(n), type(std::move(t)), nullable(nullb), primaryKey(pk), foreignKey(std::move(fk)) {}

    bool validate(const std::string& value) const {
        if (value.empty()) {
            // Порожнє значення припустиме, лише якщо колонка nullable
            return nullable;
        }
        return type->validate(value);
    }

    std::string getName() const { return name; }
    std::shared_ptr<DataType> getType() const { return type; }
    bool isPrimaryKey() const { return primaryKey; }
    bool isNullable() const { return nullable; }
    std::optional<std::pair<std::string, std::string>> getForeignKey() const { return foreignKey; }
};

class Row {
private:
    std::unordered_map<std::string, std::string> data;
public:
    Row() = default;
    explicit Row(std::unordered_map<std::string, std::string> d) : data(std::move(d)) {}
    const std::unordered_map<std::string, std::string>& getData() const { return data; }
};

class Table {
private:
    std::string name;
    std::vector<Column> columns;
    std::vector<Row> rows;
public:
    Table(const std::string& n, const std::vector<Column>& c) : name(n), columns(c) {}

    void insert(const std::unordered_map<std::string, std::string>& values) {
        std::unordered_map<std::string, std::string> rowData;

        for (const auto& col : columns) {
            auto it = values.find(col.getName());
            
            if (it == values.end()) {
                // Значення не надано
                if (col.isPrimaryKey()) {
                    throw std::runtime_error("Invalid INSERT: Missing value for PRIMARY KEY column '" + col.getName() + "'");
                }
                if (!col.isNullable()) {
                    throw std::runtime_error("Invalid INSERT: Missing value for NOT NULL column '" + col.getName() + "'");
                }
                // Це nullable колонка, значення для якої не надали. Зберігаємо порожній рядок.
                rowData[col.getName()] = "";
            } else {
                // Значення надано
                const std::string& value = it->second;
                if (!col.validate(value)) {
                    throw std::runtime_error("Invalid value '" + value + "' for column '" + col.getName() + "'");
                }
                
                // ПРИМІТКА: Тут мала б бути логіка перевірки зовнішнього ключа (Foreign Key)
                // if (col.getForeignKey().has_value()) { ... }

                rowData[col.getName()] = value;
            }
        }
        rows.emplace_back(rowData);
    }

    const std::vector<Row>& getRows() const { return rows; }

    int count(const std::string& column) const {
        int c = 0;
        for (const auto& row : rows) {
            // Рахуємо, лише якщо значення існує і не є порожнім
            auto it = row.getData().find(column);
            if (it != row.getData().end() && !it->second.empty()) {
                c++;
            }
        }
        return c;
    }

    double sum(const std::string& column) const {
        double s = 0;
        
        // Перевіряємо, чи колонка взагалі числова
        bool isNumeric = false;
        for(const auto& col : columns) {
            if(col.getName() == column) {
                if (dynamic_cast<IntegerType*>(col.getType().get())) {
                    isNumeric = true;
                }
                // Тут можна додати перевірки на FloatType, DecimalType тощо.
                break;
            }
        }

        if (!isNumeric) {
             std::cerr << "Warning: Attempted to SUM non-numeric column '" << column << "'" << std::endl;
             return 0.0;
        }

        for (const auto& row : rows) {
            auto it = row.getData().find(column);
            if (it != row.getData().end() && !it->second.empty()) {
                try {
                    s += std::stod(it->second);
                } catch (const std::exception&) {
                    // Ігноруємо невалідні дані (напр., порожній рядок) під час підрахунку суми
                }
            }
        }
        return s;
    }

    double avg(const std::string& column) const {
        int num = count(column); // Рахуємо тільки ті рядки, де є значення
        if (num == 0) return 0;
        // Правильна логіка: ділимо суму на кількість не-пустих значень
        return sum(column) / num;
    }
};

// ============================================================================
// DATABASE SINGLETON & BUILDER PATTERN
// ============================================================================

class Database {
private:
    std::unordered_map<std::string, std::shared_ptr<Table>> tables;
    Database() = default;
public:
    static Database& getInstance() {
        static Database instance;
        return instance;
    }
    
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::shared_ptr<Table> createTable(const std::string& name, const std::vector<Column>& columns) {
        if (tables.count(name)) {
            throw std::runtime_error("Table with name '" + name + "' already exists.");
        }
        auto table = std::make_shared<Table>(name, columns);
        tables[name] = table;
        return table;
    }

    std::shared_ptr<Table> getTable(const std::string& name) {
        try {
            return tables.at(name);
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Table with name '" + name + "' not found.");
        }
    }
};

class TableBuilder {
private:
    std::string name;
    std::vector<Column> columns;
public:
    explicit TableBuilder(const std::string& n) : name(n) {}

    TableBuilder& addColumn(const std::string& cname, std::shared_ptr<DataType> type,
                            bool nullable = true, bool primaryKey = false,
                            std::optional<std::pair<std::string, std::string>> fk = std::nullopt) {
        columns.emplace_back(cname, std::move(type), nullable, primaryKey, std::move(fk));
        return *this;
    }

    std::shared_ptr<Table> build() {
        return Database::getInstance().createTable(name, columns);
    }
};

// ============================================================================
// DEMONSTRATION
// ============================================================================

int main() {
    auto& db = Database::getInstance();

    auto users = TableBuilder("users")
        .addColumn("id", std::make_shared<IntegerType>(), false, true)
        .addColumn("name", std::make_shared<StringType>(), false) // NOT NULL
        .addColumn("age", std::make_shared<IntegerType>(), true)  // Nullable
        .build();

    try {
        users->insert({{"id", "1"}, {"name", "Alex"}, {"age", "25"}});
        users->insert({{"id", "2"}, {"name", "Mira"}, {"age", "30"}});
        users->insert({{"id", "3"}, {"name", "Sam"}, /* age is null */});

        std::cout << "Successfully inserted 3 users." << std::endl;

        // Тест на порушення NOT NULL
        std::cout << "\nTesting NOT NULL constraint (should fail)..." << std::endl;
        users->insert({{"id", "4"}}); 
        
    } catch (const std::exception& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    try {
        // Тест на порушення типу даних
        std::cout << "\nTesting data type constraint (should fail)..." << std::endl;
        users->insert({{"id", "four"}, {"name", "Test"}}); // "four" - не Integer
    } catch (const std::exception& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }
    
    try {
        // Тест на порушення Primary Key
        std::cout << "\nTesting PRIMARY KEY constraint (should fail)..." << std::endl;
        users->insert({{"name", "Test"}}); // "id" відсутній
    } catch (const std::exception& e) {
        std::cout << "Caught expected error: " << e.what() << std::endl;
    }

    std::cout << "\n--- Final Statistics ---" << std::endl;
    // count("age") поверне 2, оскільки у "Sam" немає віку
    std::cout << "COUNT age: " << users->count("age") << std::endl; 
    std::cout << "SUM age: " << users->sum("age") << std::endl;   // 25 + 30 = 55
    // avg("age") має бути 55 / 2 = 27.5
    std::cout << "AVG age: " << users->avg("age") << std::endl;   

    return 0;
}
