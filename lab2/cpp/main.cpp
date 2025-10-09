#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
using namespace std;

// Forward declarations
class Ship;
class Container;

class IPort {
public:
    virtual void incomingShip(Ship* s) = 0;
    virtual void outgoingShip(Ship* s) = 0;
    virtual ~IPort() {}
};

class IShip {
public:
    virtual bool sailTo(class Port* p) = 0;
    virtual void reFuel(double newFuel) = 0;
    virtual bool load(Container* c) = 0;
    virtual bool unLoad(Container* c) = 0;
    virtual ~IShip() {}
};

class Container {
protected:
    int ID;
    int weight;
public:
    Container(int id, int w) : ID(id), weight(w) {}
    virtual double consumption() const = 0;
    int getID() const { return ID; }
    int getWeight() const { return weight; }

    virtual string type() const = 0;

    bool equals(const Container& other) const {
        return (ID == other.ID && weight == other.weight && type() == other.type());
    }
    virtual ~Container() {}
};

class BasicContainer : public Container {
public:
    BasicContainer(int id, int w) : Container(id, w) {}
    double consumption() const override { return 2.5; }  // per unit weight
    string type() const override { return "Basic"; }
};

class HeavyContainer : public Container {
public:
    HeavyContainer(int id, int w) : Container(id, w) {}
    double consumption() const override { return 3.0; }
    string type() const override { return "Heavy"; }
};

class RefrigeratedContainer : public HeavyContainer {
public:
    RefrigeratedContainer(int id, int w) : HeavyContainer(id, w) {}
    double consumption() const override { return 5.0; }
    string type() const override { return "Refrigerated"; }
};

class LiquidContainer : public HeavyContainer {
public:
    LiquidContainer(int id, int w) : HeavyContainer(id, w) {}
    double consumption() const override { return 4.0; }
    string type() const override { return "Liquid"; }
};

class Port : public IPort {
    int ID;
    double latitude, longitude;
    vector<Container*> containers;
    vector<Ship*> history;
    vector<Ship*> current;
public:
    Port(int id, double lat, double lon) : ID(id), latitude(lat), longitude(lon) {}

    int getID() const { return ID; }
    double getLat() const { return latitude; }
    double getLon() const { return longitude; }

    double getDistance(const Port& other) const {
        double dx = latitude - other.latitude;
        double dy = longitude - other.longitude;
        return sqrt(dx*dx + dy*dy);
    }

    Container* getContainer(Container* c) {
    auto it = std::find_if(containers.begin(), containers.end(),
        [&](Container* x){ return x->equals(*c); });
    if (it == containers.end()) return nullptr;
    Container* found = *it;
    containers.erase(it);
    return found;
}

    void addContainer(Container* c) { containers.push_back(c); }

    void incomingShip(Ship* s) override {
        if (find(current.begin(), current.end(), s) == current.end())
            current.push_back(s);
        if (find(history.begin(), history.end(), s) == history.end())
            history.push_back(s);
    }

    void outgoingShip(Ship* s) override {
        current.erase(remove(current.begin(), current.end(), s), current.end());
    }

    void getContainersFromShip(vector<Container*> incomingContainers) {
        this->containers.insert(this->containers.end(), incomingContainers.begin(), incomingContainers.end());
    }

    void printState() const {
        cout << "Port " << ID << " (" << latitude << "," << longitude << ")\n";
        cout << " Containers: ";
        for (auto c : containers) cout << c->type() << "#" << c->getID() << " ";
        cout << "\n Ships: ";
        for (auto s : current) cout << "Ship#" << s << " "; // pointer IDs for now
        cout << "\n";
    }
};

class Ship : public IShip {
    int ID;
    double fuel;
    Port* currentPort;
    int totalWeightCapacity;
    int maxAll;
    int maxHeavy;
    int maxRefrig;
    int maxLiquid;
    double fuelPerKm;
    vector<Container*> containers;
public:
    Ship(int id, Port* port, int totalW, int all, int heavy, int refrig, int liquid, double fpkm)
        : ID(id), fuel(0), currentPort(port), totalWeightCapacity(totalW),
          maxAll(all), maxHeavy(heavy), maxRefrig(refrig), maxLiquid(liquid),
          fuelPerKm(fpkm)
    {
        if (currentPort) currentPort->incomingShip(this);
    }

    bool sailTo(Port* p) override {
        if (!p) return false;
        double dist = currentPort->getDistance(*p);
        double required = dist * (fuelPerKm + totalContainerConsumption());
        if (fuel >= required) {
            fuel -= required;
            currentPort->outgoingShip(this);
            currentPort = p;
            p->incomingShip(this); 
            return true;
        }
        return false;
    }

    void reFuel(double newFuel) override {
        fuel += newFuel;
    }

    bool load(Container* c) override {
        if ((int)containers.size() >= maxAll) return false;
        int totalW = 0;
        for (auto x : containers) totalW += x->getWeight();
        if (totalW + c->getWeight() > totalWeightCapacity) return false;

        if (c->type() == "Heavy" && countType("Heavy") >= maxHeavy) return false;
        if (c->type() == "Refrigerated" && countType("Refrigerated") >= maxRefrig) return false;
        if (c->type() == "Liquid" && countType("Liquid") >= maxLiquid) return false;

        containers.push_back(c);
        return true;
    }

    bool unLoad(Container* c) override {
        auto it = find(containers.begin(), containers.end(), c);
        if (it == containers.end()) return false;
        containers.erase(it);
        if (currentPort) currentPort->addContainer(c);
        return true;
    }

    vector<Container*> getCurrentContainers() {
        sort(containers.begin(), containers.end(),
            [](Container* a, Container* b){ return a->getID() < b->getID(); });
        return containers;
    }

    double totalContainerConsumption() const {
        double sum = 0;
        for (auto c : containers) sum += c->consumption();
        return sum;
    }

    void printState() const {
        cout << " Ship " << ID << " Fuel=" << fixed << setprecision(2) << fuel << " ";
        cout << "Containers: ";
        for (auto c : containers) cout << c->type() << "#" << c->getID() << " ";
        cout << "\n";
    }
private:
    int countType(string t) const {
        int count = 0;
        for (auto c : containers) if (c->type() == t) count++;
        return count;
    }
};

int main() {
    cout << "=== Port Management Simulation (Local Data) ===\n";

    // Create ports
    Port p1(0, 0.0, 0.0);
    Port p2(1, 10.0, 10.0);

    // Create containers
    BasicContainer c1(0, 1000);
    HeavyContainer c2(1, 5000);
    RefrigeratedContainer c3(2, 2000);
    LiquidContainer c4(3, 2500);

    p1.addContainer(&c1);
    p1.addContainer(&c2);
    p2.addContainer(&c3);
    p2.addContainer(&c4);

    // Create ships
    Ship s1(0, &p1, 20000, 5, 3, 2, 2, 0.5);
    Ship s2(1, &p2, 15000, 4, 2, 1, 1, 0.6);

    // Operations
    s1.reFuel(1000);

    cout << "\n-- Before sailing --\n";
    p1.printState();
    p2.printState();
    s1.printState();
    s1.load(p1.getContainer(&c1));
    s1.load(p1.getContainer(&c2));
    // Try sailing
    cout << "\nShip 0 tries to sail from p1 -> p2\n";
    if (s1.sailTo(&p2)) {
        cout << "Sailing success!\n";
        //p2.getContainersFromShip(s1.getCurrentContainers());
        s1.unLoad(&c1);
        s1.unLoad(&c2);
    } else
        cout << "Not enough fuel.\n";

    cout << "\n-- After sailing --\n";
    p1.printState();
    p2.printState();
    s1.printState();

    return 0;
}

