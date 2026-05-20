#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <numeric>
#include <thread>
#include <chrono>
#include <cmath>

using namespace std;

// ============================================================================
// 1. EXCEPTION HANDLING (Custom Exceptions)
// ============================================================================
class GridException : public runtime_error {
public:
    GridException(const string& msg) : runtime_error("Grid Error: " + msg) {}
};

class AuthException : public runtime_error {
public:
    AuthException(const string& msg) : runtime_error("Auth Error: " + msg) {}
};

// ============================================================================
// 2. TEMPLATES
// ============================================================================
// Template to calculate statistics (Min, Max, Average) for any numeric data type
template <typename T>
class Statistic {
private:
    vector<T> data;
public:
    void addRecord(T value) { data.push_back(value); }

    T getAverage() const {
        if (data.empty()) return 0;
        T sum = accumulate(data.begin(), data.end(), T(0));
        return sum / data.size();
    }

    T getMax() const {
        if (data.empty()) return 0;
        return *max_element(data.begin(), data.end());
    }

    T getMin() const {
        if (data.empty()) return 0;
        return *min_element(data.begin(), data.end());
    }

    void clear() { data.clear(); }
};

// ============================================================================
// 3. ABSTRACTION & INHERITANCE (Energy Sources)
// ============================================================================

enum class Weather { SUNNY, CLOUDY, WINDY, CALM, RAINY };

// Abstract Base Class
class EnergySource {
protected: // Encapsulation
    string id;
    string name;
    double maxCapacityMW;
    bool isOnline;

public:
    EnergySource(string id, string name, double cap)
        : id(id), name(name), maxCapacityMW(cap), isOnline(true) {}

    virtual ~EnergySource() = default;

    // Pure virtual function making this an Abstract Class
    virtual double generateEnergy(Weather weather, int hour) = 0;

    // Getters / Setters
    string getId() const { return id; }
    string getName() const { return name; }
    double getCapacity() const { return maxCapacityMW; }
    bool getStatus() const { return isOnline; }
    void toggleStatus() { isOnline = !isOnline; }

    // Operator Overloading (Friend function for printing)
    friend ostream& operator<<(ostream& os, const EnergySource& src) {
        os << "[" << src.id << "] " << left << setw(20) << src.name
           << " | Cap: " << setw(6) << src.maxCapacityMW << " MW | Status: "
           << (src.isOnline ? "ONLINE" : "OFFLINE");
        return os;
    }
};

// Derived Class 1
class SolarPanel : public EnergySource {
public:
    SolarPanel(string id, string name, double cap) : EnergySource(id, name, cap) {}

    // Polymorphism
    double generateEnergy(Weather weather, int hour) override {
        if (!isOnline) return 0.0;
        // Solar works best mid-day
        if (hour < 6 || hour > 18) return 0.0;

        double efficiency = 1.0;
        if (hour >= 10 && hour <= 14) efficiency = 1.0;
        else efficiency = 0.5;

        if (weather == Weather::CLOUDY || weather == Weather::RAINY) efficiency *= 0.3;

        return maxCapacityMW * efficiency;
    }
};

// Derived Class 2
class WindTurbine : public EnergySource {
public:
    WindTurbine(string id, string name, double cap) : EnergySource(id, name, cap) {}

    double generateEnergy(Weather weather, int hour) override {
        if (!isOnline) return 0.0;
        if (weather == Weather::CALM) return 0.0;
        if (weather == Weather::WINDY) return maxCapacityMW * 0.95; // High wind
        return maxCapacityMW * 0.4; // Normal wind
    }
};

// Derived Class 3
class HydroPlant : public EnergySource {
public:
    HydroPlant(string id, string name, double cap) : EnergySource(id, name, cap) {}

    double generateEnergy(Weather weather, int hour) override {
        if (!isOnline) return 0.0;
        return maxCapacityMW * 0.85; // Hydro is highly reliable base-load
    }
};

// Derived Class 4
class BiomassGenerator : public EnergySource {
public:
    BiomassGenerator(string id, string name, double cap) : EnergySource(id, name, cap) {}

    double generateEnergy(Weather weather, int hour) override {
        if (!isOnline) return 0.0;
        return maxCapacityMW * 0.90; // Controllable baseload
    }
};

// ============================================================================
// 4. STORAGE SYSTEM (Operator Overloading)
// ============================================================================
class BatteryStorage {
private:
    string id;
    double capacityMWh;
    double currentChargeMWh;

public:
    BatteryStorage(string id, double cap) : id(id), capacityMWh(cap), currentChargeMWh(cap/2) {}

    double getCharge() const { return currentChargeMWh; }
    double getCapacity() const { return capacityMWh; }
    double getPercentage() const { return (currentChargeMWh / capacityMWh) * 100.0; }

    // Operator Overloading for charging
    BatteryStorage& operator+=(double mwh) {
        currentChargeMWh = min(capacityMWh, currentChargeMWh + mwh);
        return *this;
    }

    // Operator Overloading for discharging
    BatteryStorage& operator-=(double mwh) {
        currentChargeMWh = max(0.0, currentChargeMWh - mwh);
        return *this;
    }
};

// ============================================================================
// 5. CONSUMER HIERARCHY
// ============================================================================
class Consumer {
protected:
    string id;
    string name;
    double baseDemandMW;
public:
    Consumer(string id, string name, double demand) : id(id), name(name), baseDemandMW(demand) {}
    virtual ~Consumer() = default;

    virtual double getDemand(int hour) const = 0;

    string getId() const { return id; }
    string getName() const { return name; }
};

class Residential : public Consumer {
public:
    Residential(string id, string name, double demand) : Consumer(id, name, demand) {}
    double getDemand(int hour) const override {
        // Peaks in morning (7-9) and evening (18-22)
        if ((hour >= 7 && hour <= 9) || (hour >= 18 && hour <= 22)) return baseDemandMW * 1.5;
        return baseDemandMW * 0.6;
    }
};

class Industrial : public Consumer {
public:
    Industrial(string id, string name, double demand) : Consumer(id, name, demand) {}
    double getDemand(int hour) const override {
        return baseDemandMW; // Flat, constant high demand
    }
};

// ============================================================================
// 6. COMPOSITION: THE SMART GRID
// ============================================================================
class SmartGrid {
private:
    vector<unique_ptr<EnergySource>> sources;
    vector<BatteryStorage> batteries;
    unordered_map<string, shared_ptr<Consumer>> consumers;

    Statistic<double> hourlyGenerationStats;
    Statistic<double> hourlyDemandStats;

    double totalCO2SavedTons = 0; // Cumulative

public:
    void addSource(unique_ptr<EnergySource> src) {
        sources.push_back(move(src));
    }

    void addBattery(const BatteryStorage& bat) {
        batteries.push_back(bat);
    }

    void addConsumer(shared_ptr<Consumer> cons) {
        consumers[cons->getId()] = cons; // Using unordered_map (C++11/17)
    }

    void displaySources() const {
        cout << "\n--- Connected Energy Sources ---\n";
        for (const auto& src : sources) {
            cout << *src << "\n"; // Uses overloaded <<
        }
    }

    void sortSourcesByCapacity() {
        // STL Algorithm and Lambda function
        sort(sources.begin(), sources.end(), [](const unique_ptr<EnergySource>& a, const unique_ptr<EnergySource>& b) {
            return a->getCapacity() > b->getCapacity();
        });
        cout << "Sources sorted by capacity (Descending).\n";
    }

    void simulateDay() {
        cout << "\n=== STARTING 24-HOUR GRID SIMULATION ===\n";
        hourlyGenerationStats.clear();
        hourlyDemandStats.clear();

        Weather currentWeather = Weather::SUNNY;
        double gridShortfallCount = 0;

        for (int hour = 0; hour < 24; ++hour) {
            // Dynamic weather changes
            if (hour == 12) currentWeather = Weather::WINDY;
            if (hour == 18) currentWeather = Weather::CLOUDY;

            // Calculate Generation
            double totalGen = 0.0;
            for (const auto& src : sources) {
                totalGen += src->generateEnergy(currentWeather, hour);
            }

            // Calculate Demand
            double totalDemand = 0.0;
            // C++17 Structured Bindings
            for (const auto& [id, cons] : consumers) {
                totalDemand += cons->getDemand(hour);
            }

            // Balancing & Battery Logic
            double netEnergy = totalGen - totalDemand;
            double batteryFlow = 0.0;

            if (netEnergy > 0) {
                // Excess energy -> Charge batteries
                for (auto& bat : batteries) {
                    double space = bat.getCapacity() - bat.getCharge();
                    double chargeAmt = min(netEnergy, space);
                    bat += chargeAmt; // Overloaded +=
                    netEnergy -= chargeAmt;
                    batteryFlow += chargeAmt;
                }
            } else if (netEnergy < 0) {
                // Shortfall -> Discharge batteries
                double needed = -netEnergy;
                for (auto& bat : batteries) {
                    double avail = bat.getCharge();
                    double drawAmt = min(needed, avail);
                    bat -= drawAmt; // Overloaded -=
                    needed -= drawAmt;
                    batteryFlow -= drawAmt;
                }
                if (needed > 0) gridShortfallCount++;
            }

            // Logging Stats
            hourlyGenerationStats.addRecord(totalGen);
            hourlyDemandStats.addRecord(totalDemand);

            // Calculate CO2 Saved (assume 0.4 tons of CO2 per MWh from coal replaced)
            totalCO2SavedTons += (totalGen * 0.4);

            // Real-time ASCII Dashboard
            cout << "Hr " << setfill('0') << setw(2) << hour << ":00 | "
                 << "Gen: " << fixed << setprecision(1) << setw(6) << totalGen << " MW | "
                 << "Dem: " << setw(6) << totalDemand << " MW | ";

            // Render Battery Status Graph
            if(!batteries.empty()){
                int pct = batteries[0].getPercentage();
                cout << "Bat [";
                int bars = pct / 10;
                for(int i=0; i<10; i++) cout << (i < bars ? "#" : ".");
                cout << "] " << setw(3) << pct << "%";
            }
            if (netEnergy < 0 && batteryFlow == 0) cout << "  [!!! BROWNOUT !!!]";
            cout << "\n";

            this_thread::sleep_for(chrono::milliseconds(150)); // Cinematic effect
        }
        cout << "Simulation Complete. Blackout/Brownout hours: " << gridShortfallCount << "/24\n";
    }

    void generateReport() {
        ofstream outFile("Grid_Report.txt");
        if (!outFile) throw GridException("Could not open file to save report.");

        outFile << "========================================\n";
        outFile << "      ECO-GRID DAILY SUMMARY REPORT     \n";
        outFile << "========================================\n";
        outFile << "Avg Generation: " << hourlyGenerationStats.getAverage() << " MW\n";
        outFile << "Peak Generation: " << hourlyGenerationStats.getMax() << " MW\n";
        outFile << "Avg Demand:      " << hourlyDemandStats.getAverage() << " MW\n";
        outFile << "Peak Demand:     " << hourlyDemandStats.getMax() << " MW\n";
        outFile << "Total CO2 Saved: " << totalCO2SavedTons << " Tons\n";
        outFile << "========================================\n";
        outFile.close();
        cout << "Report successfully saved to 'Grid_Report.txt'\n";
    }
};

// ============================================================================
// 7. UTILITIES & MAIN MENU
// ============================================================================

void clearCin() {
    cin.clear();
    cin.ignore(10000, '\n');
}

bool login() {
    string user, pass;
    cout << "=== ECO-GRID SYSTEM LOGIN ===\n";
    cout << "Username: "; cin >> user;
    cout << "Password: "; cin >> pass;

    if (user == "admin" && pass == "admin123") return true;
    throw AuthException("Invalid credentials. Access Denied.");
}

int main() {
    try {
        if (!login()) return 0;
    } catch (const exception& e) {
        cerr << "\n" << e.what() << "\nTerminating system.\n";
        return 1;
    }

    SmartGrid myGrid;

    // Seeding relevant data (Kenya's renewable potential)
    myGrid.addSource(make_unique<WindTurbine>("WIND-01", "Lake Turkana Wind Farm", 310.0));
    myGrid.addSource(make_unique<SolarPanel>("SOLR-01", "Garissa Solar Plant", 50.0));
    myGrid.addSource(make_unique<HydroPlant>("HYDR-01", "Seven Forks Hydro", 150.0));
    myGrid.addSource(make_unique<BiomassGenerator>("BIOM-01", "Naivasha Geo/Bio", 30.0));

    myGrid.addBattery(BatteryStorage("BAT-01", 200.0)); // 200 MWh battery

    myGrid.addConsumer(make_shared<Residential>("RES-01", "Nairobi Residential", 180.0));
    myGrid.addConsumer(make_shared<Industrial>("IND-01", "Mombasa Port/Indus", 120.0));

    int choice;
    do {
        cout << "\n=====================================";
        cout << "\n     ECO-GRID MANAGEMENT CONSOLE     ";
        cout << "\n=====================================\n";
        cout << "1. View Energy Sources\n";
        cout << "2. Sort Sources by Capacity\n";
        cout << "3. Add New Energy Source\n";
        cout << "4. Run 24-Hour Simulation\n";
        cout << "5. Generate & Save Report\n";
        cout << "0. Exit System\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            clearCin();
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        try {
            switch (choice) {
                case 1:
                    myGrid.displaySources();
                    break;
                case 2:
                    myGrid.sortSourcesByCapacity();
                    myGrid.displaySources();
                    break;
                case 3: {
                    string id, name; double cap; int type;
                    cout << "Types: 1-Solar, 2-Wind, 3-Hydro\nEnter Type: ";
                    cin >> type;
                    cout << "Enter ID (e.g., SOLR-02): "; cin >> id;
                    cout << "Enter Capacity (MW): "; cin >> cap;

                    if (type == 1) myGrid.addSource(make_unique<SolarPanel>(id, "Custom Solar", cap));
                    else if (type == 2) myGrid.addSource(make_unique<WindTurbine>(id, "Custom Wind", cap));
                    else myGrid.addSource(make_unique<HydroPlant>(id, "Custom Hydro", cap));

                    cout << "Source added successfully!\n";
                    break;
                }
                case 4:
                    myGrid.simulateDay();
                    break;
                case 5:
                    myGrid.generateReport();
                    break;
                case 0:
                    cout << "Shutting down system safely...\n";
                    break;
                default:
                    cout << "Invalid choice.\n";
            }
        } catch (const exception& e) {
            cerr << "ERROR: " << e.what() << "\n";
        }
    } while (choice != 0);

    return 0;
}

