#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <stdexcept>
#include <cctype>

using namespace std;

const string WAKE_WORD       = "hey alexa";
const string USERS_FILE      = "users.txt";
const string CMD_LOG_FILE    = "command_log.txt";
const int    MAX_APPLIANCES  = 50;

template <typename T>
class DataStore {
private:
    vector<pair<string, T>> store;
public:
    void set(const string& key, const T& value) {
        for (size_t i = 0; i < store.size(); i++) {
            if (store[i].first == key) {
                store[i].second = value;
                return;
            }
        }
        store.push_back(make_pair(key, value));
    }

    T get(const string& key) const {
        for (size_t i = 0; i < store.size(); i++) {
            if (store[i].first == key)
                return store[i].second;
        }
        throw runtime_error("Key not found in DataStore: " + key);
    }

    bool exists(const string& key) const {
        for (size_t i = 0; i < store.size(); i++)
            if (store[i].first == key) return true;
        return false;
    }

    void clear() { store.clear(); }

    int size() const { return (int)store.size(); }
};

class Appliance {
protected:
    string name;
    string makeModel;
    bool   isOn;
    int    id;

    static int applianceCount;

public:
    Appliance(const string& n, const string& mm)
        : name(n), makeModel(mm), isOn(false)
    {
        applianceCount++;
        id = applianceCount;
    }

    Appliance(const Appliance& other)
        : name(other.name), makeModel(other.makeModel),
          isOn(other.isOn), id(other.id)
    {
    }

    virtual ~Appliance() {}

    virtual void operate(const string& command) = 0;
    virtual void displayStatus() const = 0;
    virtual string getType() const = 0;

    inline string getName()      const { return name; }
    inline string getMakeModel() const { return makeModel; }
    inline bool   getIsOn()      const { return isOn; }
    inline int    getId()        const { return id; }

    inline void setName(const string& n)       { name = n; }
    inline void setMakeModel(const string& mm) { makeModel = mm; }

    void powerOn()  { isOn = true;  cout << "  [" << name << "] Powered ON.\n"; }
    void powerOff() { isOn = false; cout << "  [" << name << "] Powered OFF.\n"; }

    static int getApplianceCount() { return applianceCount; }

    string getBasicInfo() const {
        return "ID:" + to_string(id) + " | " + getType() + " | " + name +
               " | " + makeModel + " | " + (isOn ? "ON" : "OFF");
    }

    virtual string serialize() const {
        return getType() + "," + name + "," + makeModel + "," + (isOn ? "1" : "0");
    }
};

int Appliance::applianceCount = 0;

class Fan : public Appliance {
private:
    int  speed;
    bool oscillating;

public:
    Fan(const string& n, const string& mm)
        : Appliance(n, mm), speed(1), oscillating(false) {}

    void operate(const string& command) override {
        if      (command == "on")             { powerOn(); }
        else if (command == "off")            { powerOff(); speed = 0; }
        else if (command == "low")            { powerOn(); speed = 1; cout << "  [" << name << "] Speed: LOW.\n"; }
        else if (command == "medium")         { powerOn(); speed = 2; cout << "  [" << name << "] Speed: MEDIUM.\n"; }
        else if (command == "high")           { powerOn(); speed = 3; cout << "  [" << name << "] Speed: HIGH.\n"; }
        else if (command == "oscillate on")   { oscillating = true;  cout << "  [" << name << "] Oscillation ON.\n"; }
        else if (command == "oscillate off")  { oscillating = false; cout << "  [" << name << "] Oscillation OFF.\n"; }
        else cout << "  [Fan] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Fan     | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Speed: " << speed
             << " | Oscillating: " << (oscillating ? "Yes" : "No") << "\n";
    }

    string getType() const override { return "Fan"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(speed) + "," + (oscillating ? "1" : "0");
    }
};

class Light : public Appliance {
private:
    int    brightness;
    string color;

public:
    Light(const string& n, const string& mm)
        : Appliance(n, mm), brightness(100), color("warm") {}

    void operate(const string& command) override {
        if      (command == "on")          { powerOn(); }
        else if (command == "off")         { powerOff(); brightness = 0; }
        else if (command == "dim")         { powerOn(); brightness = 30; cout << "  [" << name << "] Dimmed to 30%.\n"; }
        else if (command == "bright")      { powerOn(); brightness = 100; cout << "  [" << name << "] Full brightness.\n"; }
        else if (command == "warm")        { color = "warm";     cout << "  [" << name << "] Color: Warm.\n"; }
        else if (command == "cool")        { color = "cool";     cout << "  [" << name << "] Color: Cool.\n"; }
        else if (command == "daylight")    { color = "daylight"; cout << "  [" << name << "] Color: Daylight.\n"; }
        else {
            if (command.size() > 11 && command.substr(0, 10) == "brightness") {
                try {
                    int val = stoi(command.substr(11));
                    if (val < 0 || val > 100) throw out_of_range("Brightness must be 0-100");
                    brightness = val;
                    powerOn();
                    cout << "  [" << name << "] Brightness set to " << brightness << "%.\n";
                } catch (...) {
                    cout << "  [Light] Invalid brightness value.\n";
                }
            } else {
                cout << "  [Light] Unknown command: " << command << "\n";
            }
        }
    }

    void displayStatus() const override {
        cout << "  Light   | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Brightness: " << brightness << "% | Color: " << color << "\n";
    }

    string getType() const override { return "Light"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(brightness) + "," + color;
    }
};

class AC : public Appliance {
private:
    int    temperature;
    string mode;
    int    fanSpeed;

public:
    AC(const string& n, const string& mm)
        : Appliance(n, mm), temperature(24), mode("cool"), fanSpeed(2) {}

    void operate(const string& command) override {
        if      (command == "on")   { powerOn(); }
        else if (command == "off")  { powerOff(); }
        else if (command == "cool") { mode = "cool"; cout << "  [" << name << "] Mode: COOL.\n"; }
        else if (command == "heat") { mode = "heat"; cout << "  [" << name << "] Mode: HEAT.\n"; }
        else if (command == "fan")  { mode = "fan";  cout << "  [" << name << "] Mode: FAN.\n"; }
        else if (command == "auto") { mode = "auto"; cout << "  [" << name << "] Mode: AUTO.\n"; }
        else if (command.size() > 4 && command.substr(0, 4) == "temp") {
            try {
                int val = stoi(command.substr(5));
                if (val < 16 || val > 32) throw out_of_range("Temperature must be 16-32C");
                temperature = val;
                cout << "  [" << name << "] Temperature set to " << temperature << "C.\n";
            } catch (out_of_range& e) {
                cout << "  [AC] Error: " << e.what() << "\n";
            } catch (...) {
                cout << "  [AC] Invalid temperature.\n";
            }
        }
        else cout << "  [AC] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  AC      | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Temp: " << temperature << "C | Mode: " << mode << "\n";
    }

    string getType() const override { return "AC"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(temperature) + "," + mode + "," + to_string(fanSpeed);
    }
};

class Door : public Appliance {
private:
    const string location;
    string state;

public:
    Door(const string& n, const string& mm, const string& loc)
        : Appliance(n, mm), location(loc), state("closed") {}

    void operate(const string& command) override {
        if      (command == "open")   { state = "open";   powerOn();  cout << "  [" << name << "] Door OPENED.\n"; }
        else if (command == "half")   { state = "half";   powerOn();  cout << "  [" << name << "] Door HALF-OPEN.\n"; }
        else if (command == "close")  { state = "closed"; powerOff(); cout << "  [" << name << "] Door CLOSED.\n"; }
        else if (command == "lock")   { state = "locked"; powerOff(); cout << "  [" << name << "] Door LOCKED.\n"; }
        else if (command == "unlock") { state = "closed"; cout << "  [" << name << "] Door UNLOCKED.\n"; }
        else cout << "  [Door] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Door    | " << name << " | State: " << state
             << " | Location: " << location << "\n";
    }

    string getLocation() const { return location; }

    string getType() const override { return "Door"; }

    string serialize() const override {
        return Appliance::serialize() + "," + location + "," + state;
    }
};

class Curtain : public Appliance {
private:
    int openPercent;

public:
    Curtain(const string& n, const string& mm)
        : Appliance(n, mm), openPercent(0) {}

    void operate(const string& command) override {
        if      (command == "open")  { openPercent = 100; powerOn();  cout << "  [" << name << "] Curtains FULLY OPEN.\n"; }
        else if (command == "close") { openPercent = 0;   powerOff(); cout << "  [" << name << "] Curtains CLOSED.\n"; }
        else if (command == "half")  { openPercent = 50;  powerOn();  cout << "  [" << name << "] Curtains HALF-OPEN.\n"; }
        else if (command.size() > 5 && command.substr(0, 4) == "open") {
            try {
                int val = stoi(command.substr(5));
                if (val < 0 || val > 100) throw out_of_range("Value must be 0-100");
                openPercent = val;
                cout << "  [" << name << "] Curtains opened to " << val << "%.\n";
            } catch (...) {
                cout << "  [Curtain] Invalid percentage.\n";
            }
        }
        else cout << "  [Curtain] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Curtain | " << name << " | " << (isOn ? "Open" : "Closed")
             << " | Open: " << openPercent << "%\n";
    }

    string getType() const override { return "Curtain"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(openPercent);
    }
};

class Fridge : public Appliance {
private:
    int  fridgeTemp;
    int  freezerTemp;
    bool icemaker;

public:
    Fridge(const string& n, const string& mm)
        : Appliance(n, mm), fridgeTemp(4), freezerTemp(-18), icemaker(false) {}

    void operate(const string& command) override {
        if      (command == "on")      { powerOn(); }
        else if (command == "off")     { powerOff(); }
        else if (command == "ice on")  { icemaker = true;  cout << "  [" << name << "] Ice maker ON.\n"; }
        else if (command == "ice off") { icemaker = false; cout << "  [" << name << "] Ice maker OFF.\n"; }
        else if (command.size() > 6 && command.substr(0, 5) == "ftemp") {
            try {
                int v = stoi(command.substr(6));
                if (v < 0 || v > 10) throw out_of_range("Fridge temp should be 0-10C");
                fridgeTemp = v;
                cout << "  [" << name << "] Fridge temp: " << fridgeTemp << "C\n";
            } catch (out_of_range& e) { cout << "  Error: " << e.what() << "\n"; }
              catch (...)             { cout << "  Invalid value.\n"; }
        }
        else cout << "  [Fridge] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Fridge  | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Fridge: " << fridgeTemp << "C | Freezer: " << freezerTemp
             << "C | Ice: " << (icemaker ? "On" : "Off") << "\n";
    }

    string getType() const override { return "Fridge"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(fridgeTemp) + ","
               + to_string(freezerTemp) + "," + (icemaker ? "1" : "0");
    }
};

class Oven : public Appliance {
private:
    int    temperature;
    string mode;
    int    timerMinutes;

public:
    Oven(const string& n, const string& mm)
        : Appliance(n, mm), temperature(180), mode("bake"), timerMinutes(0) {}

    void operate(const string& command) override {
        if      (command == "on")         { powerOn(); }
        else if (command == "off")        { powerOff(); timerMinutes = 0; }
        else if (command == "bake")       { mode = "bake";      cout << "  [" << name << "] Mode: BAKE.\n"; }
        else if (command == "grill")      { mode = "grill";     cout << "  [" << name << "] Mode: GRILL.\n"; }
        else if (command == "microwave")  { mode = "microwave"; cout << "  [" << name << "] Mode: MICROWAVE.\n"; }
        else if (command == "toast")      { mode = "toast";     cout << "  [" << name << "] Mode: TOAST.\n"; }
        else if (command.size() > 5 && command.substr(0, 4) == "temp") {
            try {
                int v = stoi(command.substr(5));
                if (v < 50 || v > 300) throw out_of_range("Temp must be 50-300C");
                temperature = v;
                cout << "  [" << name << "] Oven temp: " << temperature << "C\n";
            } catch (out_of_range& e) { cout << "  Error: " << e.what() << "\n"; }
              catch (...)             { cout << "  Invalid temp.\n"; }
        }
        else if (command.size() > 6 && command.substr(0, 5) == "timer") {
            try {
                timerMinutes = stoi(command.substr(6));
                cout << "  [" << name << "] Timer set: " << timerMinutes << " min.\n";
            } catch (...) { cout << "  Invalid timer.\n"; }
        }
        else cout << "  [Oven] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Oven    | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Mode: " << mode << " | Temp: " << temperature
             << "C | Timer: " << timerMinutes << " min\n";
    }

    string getType() const override { return "Oven"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(temperature)
               + "," + mode + "," + to_string(timerMinutes);
    }
};

class Geyser : public Appliance {
private:
    int  targetTemp;
    bool ecoMode;

public:
    Geyser(const string& n, const string& mm)
        : Appliance(n, mm), targetTemp(60), ecoMode(false) {}

    void operate(const string& command) override {
        if      (command == "on")      { powerOn(); }
        else if (command == "off")     { powerOff(); }
        else if (command == "eco on")  { ecoMode = true;  cout << "  [" << name << "] Eco Mode ON.\n"; }
        else if (command == "eco off") { ecoMode = false; cout << "  [" << name << "] Eco Mode OFF.\n"; }
        else if (command.size() > 5 && command.substr(0, 4) == "temp") {
            try {
                int v = stoi(command.substr(5));
                if (v < 30 || v > 85) throw out_of_range("Temp must be 30-85C");
                targetTemp = v;
                cout << "  [" << name << "] Geyser target: " << targetTemp << "C\n";
            } catch (out_of_range& e) { cout << "  Error: " << e.what() << "\n"; }
              catch (...)             { cout << "  Invalid temp.\n"; }
        }
        else cout << "  [Geyser] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Geyser  | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Target: " << targetTemp << "C | Eco: " << (ecoMode ? "On" : "Off") << "\n";
    }

    string getType() const override { return "Geyser"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(targetTemp) + "," + (ecoMode ? "1" : "0");
    }
};

class Thermostat : public Appliance {
private:
    int    setPoint;
    int    currentTemp;
    string mode;

public:
    Thermostat(const string& n, const string& mm)
        : Appliance(n, mm), setPoint(22), currentTemp(25), mode("auto") {}

    void operate(const string& command) override {
        if      (command == "on")   { powerOn(); }
        else if (command == "off")  { powerOff(); mode = "off"; }
        else if (command == "heat") { mode = "heat"; cout << "  [" << name << "] Thermostat: HEAT mode.\n"; }
        else if (command == "cool") { mode = "cool"; cout << "  [" << name << "] Thermostat: COOL mode.\n"; }
        else if (command == "auto") { mode = "auto"; cout << "  [" << name << "] Thermostat: AUTO mode.\n"; }
        else if (command.size() > 4 && command.substr(0, 3) == "set") {
            try {
                int v = stoi(command.substr(4));
                if (v < 10 || v > 35) throw out_of_range("Set point must be 10-35C");
                setPoint = v;
                cout << "  [" << name << "] Set point: " << setPoint << "C\n";
            } catch (out_of_range& e) { cout << "  Error: " << e.what() << "\n"; }
              catch (...)             { cout << "  Invalid value.\n"; }
        }
        else cout << "  [Thermostat] Unknown command: " << command << "\n";
    }

    void displayStatus() const override {
        cout << "  Thermo  | " << name << " | " << (isOn ? "ON" : "OFF")
             << " | Set: " << setPoint << "C | Current: " << currentTemp
             << "C | Mode: " << mode << "\n";
    }

    string getType() const override { return "Thermostat"; }

    string serialize() const override {
        return Appliance::serialize() + "," + to_string(setPoint)
               + "," + to_string(currentTemp) + "," + mode;
    }
};

class Device {
protected:
    string deviceId;
public:
    Device(const string& id) : deviceId(id) {}
    virtual string getDeviceId() const { return deviceId; }
    virtual ~Device() {}
};

class Connectable : virtual public Device {
public:
    bool isConnected;
    Connectable() : Device(""), isConnected(false) {}
    Connectable(const string& id) : Device(id), isConnected(false) {}
    void connect()    { isConnected = true;  cout << "  Device connected to network.\n"; }
    void disconnect() { isConnected = false; cout << "  Device disconnected.\n"; }
};

class Controllable : virtual public Device {
public:
    bool isRemoteEnabled;
    Controllable() : Device(""), isRemoteEnabled(false) {}
    Controllable(const string& id) : Device(id), isRemoteEnabled(false) {}
    void enableRemote()  { isRemoteEnabled = true;  cout << "  Remote control ENABLED.\n"; }
    void disableRemote() { isRemoteEnabled = false; cout << "  Remote control DISABLED.\n"; }
};

class SmartDevice : public Connectable, public Controllable {
private:
    string smartName;
public:
    SmartDevice(const string& id, const string& nm)
        : Device(id), Connectable(), Controllable(), smartName(nm)
    {
        isConnected     = false;
        isRemoteEnabled = false;
    }

    void showInfo() const {
        cout << "  SmartDevice: " << smartName
             << " | ID: " << deviceId
             << " | Connected: " << (isConnected ? "Yes" : "No")
             << " | Remote: " << (isRemoteEnabled ? "Yes" : "No") << "\n";
    }
};

class User {
private:
    string   name;
    int      age;
    string   dob;
    string   email;
    string   password;

    static int userCount;

    vector<Appliance*> appliances;
    list<string> commandHistory;
    DataStore<string> stateCache;

public:
    User() : name(""), age(0), dob(""), email(""), password("") {
        userCount++;
    }

    User(const string& n, int a, const string& d,
         const string& em, const string& pw)
        : name(n), age(a), dob(d), email(em), password(pw)
    {
        userCount++;
    }

    User(const User& other)
        : name(other.name), age(other.age), dob(other.dob),
          email(other.email), password(other.password)
    {
        userCount++;
    }

    ~User() {
        for (size_t i = 0; i < appliances.size(); i++)
            delete appliances[i];
        appliances.clear();
        userCount--;
    }

    inline string getName()  const { return name; }
    inline int    getAge()   const { return age; }
    inline string getDob()   const { return dob; }
    inline string getEmail() const { return email; }

    void setName(const string& n)      { name = n; }
    void setAge(int a)                 { this->age = a; }
    void setDob(const string& d)       { this->dob = d; }
    void setEmail(const string& em)    { this->email = em; }
    void setPassword(const string& pw) { this->password = pw; }

    bool checkPassword(const string& pw) const { return password == pw; }

    static int getUserCount() { return userCount; }

    void addAppliance(Appliance* a) {
        appliances.push_back(a);
        cout << "  Appliance '" << a->getName() << "' added.\n";
    }

    bool removeAppliance(const string& appName) {
        for (vector<Appliance*>::iterator it = appliances.begin(); it != appliances.end(); ++it) {
            if ((*it)->getName() == appName) {
                delete *it;
                appliances.erase(it);
                cout << "  Appliance '" << appName << "' removed.\n";
                return true;
            }
        }
        cout << "  Appliance '" << appName << "' not found.\n";
        return false;
    }

    Appliance* findAppliance(const string& appName) {
        for (size_t i = 0; i < appliances.size(); i++) {
            if (appliances[i]->getName() == appName) return appliances[i];
        }
        return NULL;
    }

    void listAppliances() const {
        if (appliances.empty()) {
            cout << "  No appliances registered.\n";
            return;
        }
        cout << "\n  === Your Appliances ===\n";
        for (size_t i = 0; i < appliances.size(); i++)
            appliances[i]->displayStatus();
    }

    void logCommand(const string& cmd) {
        commandHistory.push_back(cmd);
        if (commandHistory.size() > 20)
            commandHistory.pop_front();
    }

    void showHistory() const {
        cout << "\n  === Command History ===\n";
        if (commandHistory.empty()) { cout << "  No history.\n"; return; }
        for (list<string>::const_iterator it = commandHistory.begin(); it != commandHistory.end(); ++it)
            cout << "  > " << *it << "\n";
    }

    const vector<Appliance*>& getAppliances() const { return appliances; }

    void saveAppliancesToFile() const {
        string filename = email + "_appliances.txt";
        for (size_t i = 0; i < filename.size(); i++)
            if (filename[i] == '@' || filename[i] == '.') filename[i] = '_';

        ofstream file(filename.c_str());
        if (!file.is_open()) {
            cerr << "  [File Error] Cannot open " << filename << "\n";
            return;
        }
        for (size_t i = 0; i < appliances.size(); i++)
            file << appliances[i]->serialize() << "\n";
        file.close();
        cout << "  Appliances saved to " << filename << "\n";
    }

    void loadAppliancesFromFile() {
        string filename = email + "_appliances.txt";
        for (size_t i = 0; i < filename.size(); i++)
            if (filename[i] == '@' || filename[i] == '.') filename[i] = '_';

        ifstream file(filename.c_str());
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string type, name, makeModel, onStr;

            getline(ss, type,      ',');
            getline(ss, name,      ',');
            getline(ss, makeModel, ',');
            getline(ss, onStr,     ',');
            bool on = (onStr == "1");

            Appliance* a = NULL;
            if      (type == "Fan")        a = new Fan(name, makeModel);
            else if (type == "Light")      a = new Light(name, makeModel);
            else if (type == "AC")         a = new AC(name, makeModel);
            else if (type == "Door")       a = new Door(name, makeModel, "unknown");
            else if (type == "Curtain")    a = new Curtain(name, makeModel);
            else if (type == "Fridge")     a = new Fridge(name, makeModel);
            else if (type == "Oven")       a = new Oven(name, makeModel);
            else if (type == "Geyser")     a = new Geyser(name, makeModel);
            else if (type == "Thermostat") a = new Thermostat(name, makeModel);

            if (a) {
                if (on) a->powerOn();
                appliances.push_back(a);
            }
        }
        file.close();
        cout << "  Appliances loaded from " << filename << "\n";
    }
};

int User::userCount = 0;

class UserManager {
private:
    vector<User*> users;

public:
    UserManager() { loadUsersFromFile(); }

    ~UserManager() {
        saveUsersToFile();
        for (size_t i = 0; i < users.size(); i++) delete users[i];
    }

    void registerUser() {
        string name, dob, email, pw, pw2;
        int age;

        cout << "\n  === Create Account ===\n";
        cout << "  Full Name   : "; cin.ignore(1000, '\n'); getline(cin, name);
        cout << "  Age         : "; cin >> age;
        cout << "  Date of Birth (DD/MM/YYYY): "; cin >> dob;
        cout << "  Email       : "; cin >> email;

        try {
            for (size_t i = 0; i < users.size(); i++)
                if (users[i]->getEmail() == email)
                    throw runtime_error("Email already registered: " + email);
        } catch (const runtime_error& e) {
            cout << "  [Error] " << e.what() << "\n";
            return;
        }

        cout << "  Password    : "; cin >> pw;
        cout << "  Confirm Pw  : "; cin >> pw2;

        try {
            if (pw != pw2) throw invalid_argument("Passwords do not match.");
            if (pw.length() < 6) throw invalid_argument("Password too short (min 6 chars).");
        } catch (const invalid_argument& e) {
            cout << "  [Error] " << e.what() << "\n";
            return;
        }

        User* newUser = new User(name, age, dob, email, pw);
        users.push_back(newUser);
        saveUsersToFile();
        cout << "  Account created! Welcome, " << name << "!\n";
    }

    User* login() {
        string email, pw;
        cout << "\n  === Login ===\n";
        cout << "  Email   : "; cin >> email;
        cout << "  Password: "; cin >> pw;

        for (size_t i = 0; i < users.size(); i++) {
            if (users[i]->getEmail() == email && users[i]->checkPassword(pw)) {
                cout << "  Welcome back, " << users[i]->getName() << "!\n";
                return users[i];
            }
        }
        cout << "  [Error] Invalid credentials.\n";
        return NULL;
    }

    void saveUsersToFile() const {
        ofstream file(USERS_FILE.c_str());
        if (!file.is_open()) { cerr << "[File Error] Cannot write users.txt\n"; return; }
        for (size_t i = 0; i < users.size(); i++)
            file << users[i]->getName() << "," << users[i]->getAge() << ","
                 << users[i]->getDob()  << "," << users[i]->getEmail() << "\n";
        file.close();
    }

    void loadUsersFromFile() {
        ifstream file(USERS_FILE.c_str());
        if (!file.is_open()) return;
        file.close();
    }

    int getUserCount() const { return (int)users.size(); }
};

void printSeparator() {
    cout << string(55, '-') << "\n";
}

void printSeparator(char ch, int len) {
    cout << string(len, ch) << "\n";
}

void printSeparator(const string& title) {
    int pad = (55 - (int)title.length()) / 2;
    if (pad < 0) pad = 0;
    cout << string(pad, '=') << " " << title << " " << string(pad, '=') << "\n";
}

pair<string, string> parseCommand(const string& input) {
    size_t pos = input.find('|');
    if (pos == string::npos) return make_pair(string(""), input);
    string appName = input.substr(0, pos);
    string cmd     = input.substr(pos + 1);

    while (!appName.empty() && appName[appName.size()-1] == ' ')
        appName.erase(appName.size()-1);

    while (!appName.empty() && appName[0] == ' ')
        appName.erase(0, 1);

    while (!cmd.empty() && cmd[cmd.size()-1] == ' ')
        cmd.erase(cmd.size()-1);

    while (!cmd.empty() && cmd[0] == ' ')
        cmd.erase(0, 1);

    return make_pair(appName, cmd);
}

string toLower(const string& s) {
    string result = s;
    for (size_t i = 0; i < result.size(); i++)
        result[i] = (char)tolower((unsigned char)result[i]);
    return result;
}

void demoArrayOfObjects() {
    printSeparator("Array of Objects Demo");
    Fan fans[3] = {
        Fan("Bedroom Fan", "Orient 56\""),
        Fan("Kitchen Fan", "GFC 48\""),
        Fan("Lounge Fan",  "Havells 52\"")
    };
    for (int i = 0; i < 3; i++) {
        fans[i].powerOn();
        fans[i].displayStatus();
    }
    printSeparator();
}

void demoDiamondProblem() {
    printSeparator("Diamond Problem Demo (Virtual Inheritance)");
    SmartDevice sd("SD-001", "Smart Hub Pro");
    sd.connect();
    sd.enableRemote();
    sd.showInfo();

    cout << "  Device ID (unambiguous): " << sd.getDeviceId() << "\n";
    printSeparator();
}

void userSession(User* user) {
    user->loadAppliancesFromFile();

    string input;
    bool running = true;

    while (running) {
        printSeparator("Smart Home - " + user->getName());
        cout << "  Say: '" << WAKE_WORD << " <appliance> | <command>'\n";
        cout << "  Or type a menu option:\n";
        cout << "  [1] List Appliances    [2] Add Appliance\n";
        cout << "  [3] Remove Appliance   [4] Command History\n";
        cout << "  [5] Save & Logout      [6] Total Appliances: "
             << Appliance::getApplianceCount() << "\n";
        cout << "  [7] Array Demo         [8] Diamond Demo\n";
        printSeparator();
        cout << "  > ";

        cin.ignore(1000, '\n');
        getline(cin, input);
        input = toLower(input);

        if (input == "5" || input == "logout") {
            user->saveAppliancesToFile();
            cout << "  Saved. Goodbye, " << user->getName() << "!\n";
            running = false;

        } else if (input == "1") {
            user->listAppliances();

        } else if (input == "2") {
            printSeparator("Add Appliance");
            cout << "  Types: fan / light / ac / door / curtain / fridge / oven / geyser / thermostat\n";
            cout << "  Type  : ";
            string type; getline(cin, type); type = toLower(type);
            cout << "  Name  : ";
            string name; getline(cin, name);
            cout << "  Make/Model: ";
            string mm; getline(cin, mm);

            Appliance* a = NULL;

            try {
                if      (type == "fan")        a = new Fan(name, mm);
                else if (type == "light")      a = new Light(name, mm);
                else if (type == "ac")         a = new AC(name, mm);
                else if (type == "door")     {
                    string loc;
                    cout << "  Location: "; getline(cin, loc);
                    a = new Door(name, mm, loc);
                }
                else if (type == "curtain")    a = new Curtain(name, mm);
                else if (type == "fridge")     a = new Fridge(name, mm);
                else if (type == "oven")       a = new Oven(name, mm);
                else if (type == "geyser")     a = new Geyser(name, mm);
                else if (type == "thermostat") a = new Thermostat(name, mm);
                else throw invalid_argument("Unknown appliance type: " + type);
            } catch (const invalid_argument& e) {
                cout << "  [Error] " << e.what() << "\n";
            }

            if (a) user->addAppliance(a);

        } else if (input == "3") {
            cout << "  Appliance name to remove: ";
            string name; getline(cin, name);
            user->removeAppliance(name);

        } else if (input == "4") {
            user->showHistory();

        } else if (input == "6") {
            cout << "  Total appliances ever created: " << Appliance::getApplianceCount() << "\n";

        } else if (input == "7") {
            demoArrayOfObjects();

        } else if (input == "8") {
            demoDiamondProblem();

        } else if (input.size() >= WAKE_WORD.size() &&
                   input.substr(0, WAKE_WORD.size()) == WAKE_WORD) {

            string rest = input.substr(WAKE_WORD.size());
            while (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);

            if (rest == "turn off all" || rest == "goodnight") {
                cout << "  Turning off all appliances...\n";
                const vector<Appliance*>& apps = user->getAppliances();
                for (size_t i = 0; i < apps.size(); i++) apps[i]->powerOff();
                user->logCommand(rest);
                continue;
            }
            if (rest == "turn on all") {
                cout << "  Turning on all appliances...\n";
                const vector<Appliance*>& apps = user->getAppliances();
                for (size_t i = 0; i < apps.size(); i++) apps[i]->powerOn();
                user->logCommand(rest);
                continue;
            }
            if (rest == "status" || rest == "list") {
                user->listAppliances();
                continue;
            }

            pair<string,string> parsed = parseCommand(rest);
            string appName = parsed.first;
            string cmd     = parsed.second;

            if (appName.empty()) {
                cout << "  Use format: " << WAKE_WORD << " <appliance name> | <command>\n";
                continue;
            }

            Appliance* target = user->findAppliance(appName);
            if (!target) {
                cout << "  [Error] Appliance '" << appName << "' not found.\n";
            } else {
                target->operate(cmd);
                user->logCommand(WAKE_WORD + " " + appName + " | " + cmd);
            }

        } else {
            cout << "  [?] Unknown input. Try '" << WAKE_WORD << " <appliance> | <command>'\n";
        }
    }
}

int main() {
    printSeparator("SMART HOME CONTROL SYSTEM");
    cout << "  Powered by C++ OOP\n";
    printSeparator();

    UserManager mgr;
    bool appRunning = true;

    while (appRunning) {
        printSeparator("Main Menu");
        cout << "  [1] Register\n";
        cout << "  [2] Login\n";
        cout << "  [3] Exit\n";
        cout << "  > ";

        int choice;

        try {
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                throw invalid_argument("Please enter 1, 2 or 3.");
            }
        } catch (const invalid_argument& e) {
            cout << "  [Error] " << e.what() << "\n";
            continue;
        }

        switch (choice) {
            case 1:
                mgr.registerUser();
                break;

            case 2: {
                User* loggedIn = mgr.login();
                if (loggedIn) {
                    userSession(loggedIn);
                }
                break;
            }

            case 3:
                cout << "  Goodbye!\n";
                appRunning = false;
                break;

            default:
                cout << "  [Error] Invalid option.\n";
        }
    }

    return 0;
}