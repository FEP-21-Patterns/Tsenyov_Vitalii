#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <iomanip>

using namespace std;
class Customer;

class Bill {
private:
    double limitingAmount;
    double currentDebt;
public:
    Bill(double limit) : limitingAmount(limit), currentDebt(0.0) {}

    bool check(double amount) {
        return (currentDebt + amount <= limitingAmount);
    }

    void add(double amount) {
        currentDebt += amount;
    }

    void pay(double amount) {
        if (amount > currentDebt) amount = currentDebt;
        currentDebt -= amount;
    }

    void changeTheLimit(double amount) {
        limitingAmount = amount;
    }

    double getLimitingAmount() const { return limitingAmount; }
    double getCurrentDebt() const { return currentDebt; }
};

class Operator;

class Operator {
private:
    int ID;
    double talkingCharge;
    double messageCost;
    double networkCharge;
    int discountRate; // percentage
public:
    Operator(int ID, double talk, double msg, double net, int discount)
        : ID(ID), talkingCharge(talk), messageCost(msg),
          networkCharge(net), discountRate(discount) {}

    double calculateTalkingCost(int minute, const Customer& customer) const;
    double calculateMessageCost(int quantity, const Customer& customer, const Customer& other) const;
    double calculateNetworkCost(double amount) const;

    double getTalkingCharge() const { return talkingCharge; }
    double getMessageCost() const { return messageCost; }
    double getNetworkCharge() const { return networkCharge; }
    int getDiscountRate() const { return discountRate; }
    int getId() const { return ID; }
};

class Customer {
private:
    int ID;
    string name;
    int age;
    Operator* op; // aggregation: customer uses an operator
    Bill* bill;   // aggregation: customer has a bill
public:
    Customer(int ID, string name, int age, Operator* op, Bill* bill, double limit)
        : ID(ID), name(name), age(age), op(op), bill(bill) {}

    void talk(int minute, Customer& other) {
        double cost = op->calculateTalkingCost(minute, *this);
        if (bill->check(cost)) bill->add(cost);
        else cout << "Limit exceeded. Talk not allowed.\n";
    }

    void message(int quantity, Customer& other) {
        double cost = op->calculateMessageCost(quantity, *this, other);
        if (bill->check(cost)) bill->add(cost);
        else cout << "Limit exceeded. Message not sent.\n";
    }

    void connection(double amount) {
        double cost = op->calculateNetworkCost(amount);
        if (bill->check(cost)) bill->add(cost);
        else cout << "Limit exceeded. Connection denied.\n";
    }

    void pay(double amount) {
        bill->pay(amount);
    }

    void changeOperator(Operator* newOp) {
        op = newOp;
    }

    void changeBillLimit(double newLimit) {
        bill->changeTheLimit(newLimit);
    }
    int getAge() const { return age; }
    int getId() const { return ID; }
    string getName() const { return name; }
    Operator* getOperator() const { return op; }
    Bill* getBill() const { return bill; }
};

double Operator::calculateTalkingCost(int minute, const Customer& customer) const {
    double cost = talkingCharge * minute;
    int age = customer.getAge();
    if (age < 18 || age > 65) cost *= (1 - discountRate / 100.0);
    return cost;
}

double Operator::calculateMessageCost(int quantity, const Customer& customer, const Customer& other) const {
    double cost = messageCost * quantity;
    if (customer.getOperator()->getId() == other.getOperator()->getId()) {
        cost *= (1 - discountRate / 100.0);
    }
    return cost;
}

double Operator::calculateNetworkCost(double amount) const {
    return networkCharge * amount;
}

int main() {
    vector<unique_ptr<Customer>> customers;
    vector<unique_ptr<Operator>> operators;
    vector<unique_ptr<Bill>> bills;

    operators.push_back(make_unique<Operator>(0, 0.5, 0.2, 0.1, 10));
    operators.push_back(make_unique<Operator>(1, 0.6, 0.25, 0.15, 15));

    bills.push_back(make_unique<Bill>(100.0));
    bills.push_back(make_unique<Bill>(150.0));

    customers.push_back(make_unique<Customer>(0, "Alice", 20, operators[0].get(), bills[0].get(), 100.0));
    customers.push_back(make_unique<Customer>(1, "Bob", 70, operators[1].get(), bills[1].get(), 150.0));

    cout << "=== Initial State ===\n\n";

    cout << "--- Operators ---\n";
    for (auto& op : operators) {
        cout << "Operator " << op->getId()
             << " | Talk=" << op->getTalkingCharge()
             << " | Msg=" << op->getMessageCost()
             << " | Net=" << op->getNetworkCharge()
             << " | Discount=" << op->getDiscountRate() << "%\n";
    }

    cout << "\n--- Customers ---\n";
    for (auto& c : customers) {
        cout << "Customer " << c->getId()
             << " (" << c->getName() << ", age=" << c->getAge() << ")\n"
             << "   Operator ID: " << c->getOperator()->getId() << "\n"
             << "   Bill -> Debt: " << fixed << setprecision(2)
             << c->getBill()->getCurrentDebt()
             << " | Limit: " << c->getBill()->getLimitingAmount()
             << "\n\n";
    }

    cout << "=============================\n\n";

    // ---- Simulation ----
    cout << "[Operation] Alice talks to Bob for 10 minutes.\n";
    customers[0]->talk(10, *customers[1]);

    cout << "[Operation] Bob sends 5 messages to Alice.\n";
    customers[1]->message(5, *customers[0]);

    cout << "[Operation] Alice connects to the internet with 50 MB.\n";
    customers[0]->connection(50);

    cout << "[Operation] Alice pays 20 towards her bill.\n";
    customers[0]->pay(20);

    cout << "[Operation] Alice changes operator to Operator 1.\n";
    customers[0]->changeOperator(operators[1].get());

    cout << "\n=== Final State ===\n\n";

    cout << "--- Operators ---\n";
    for (auto& op : operators) {
        cout << "Operator " << op->getId()
             << " | Talk=" << op->getTalkingCharge()
             << " | Msg=" << op->getMessageCost()
             << " | Net=" << op->getNetworkCharge()
             << " | Discount=" << op->getDiscountRate() << "%\n";
    }

    cout << "\n--- Customers ---\n";
    for (auto& c : customers) {
        cout << "Customer " << c->getId()
             << " (" << c->getName() << ", age=" << c->getAge() << ")\n"
             << "   Operator ID: " << c->getOperator()->getId() << "\n"
             << "   Bill -> Debt: " << fixed << setprecision(2)
             << c->getBill()->getCurrentDebt()
             << " | Limit: " << c->getBill()->getLimitingAmount()
             << "\n\n";
    }

    return 0;
}
