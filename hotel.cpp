// Hotel Reservation System - CSE 202 OOP Project
// Manages room bookings, availability, customer records, services,
// billing (discount + GST), checkout history and revenue reports.

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <stdexcept>
using namespace std;

// ======================= Helper (input) functions =========================
string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(tolower(ch));
    });
    return s;
}

string readLine(const string& prompt) {
    string s;
    cout << prompt;
    if (!getline(cin, s)) {
        throw runtime_error("Input stream closed.");
    }
    replace(s.begin(), s.end(), '|', '/'); // '|' is our file separator
    return s;
}

int readInt(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos;
            int v = stoi(s, &pos);
            if (pos == s.size()) return v;
        } catch (...) {}
        cout << "Please enter a valid number.\n";
    }
}

bool validPhone(const string& p) {
    return p.size() == 10 && all_of(p.begin(), p.end(), [](unsigned char ch) {
        return isdigit(ch) != 0;
    });
}

// Validates a real calendar date in DD-MM-YYYY format, including leap years.
bool validDate(const string& d) {
    if (d.size() != 10 || d[2] != '-' || d[5] != '-') return false;
    for (size_t i = 0; i < d.size(); ++i) {
        if (i == 2 || i == 5) continue;
        if (!isdigit(static_cast<unsigned char>(d[i]))) return false;
    }

    int day = stoi(d.substr(0, 2));
    int month = stoi(d.substr(3, 2));
    int year = stoi(d.substr(6, 4));
    if (year < 1 || month < 1 || month > 12 || day < 1) return false;

    const bool leapYear = (year % 400 == 0) ||
                          (year % 4 == 0 && year % 100 != 0);
    const int daysInMonth[] = {31, leapYear ? 29 : 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};
    return day <= daysInMonth[month - 1];
}

// ======================= Room hierarchy ===================================
// ABSTRACTION: Room defines the common interface for every room type.
// ENCAPSULATION: room state is private and accessed through public methods.
class Room {
private:
    int roomNumber;
    bool occupied;
public:
    Room(int number) : roomNumber(number), occupied(false) {}
    virtual ~Room() {}

    virtual string getType() const = 0;
    virtual double getPricePerNight() const = 0;
    virtual string getAmenities() const = 0;

    int getNumber() const { return roomNumber; }
    bool isOccupied() const { return occupied; }
    void setOccupied(bool status) { occupied = status; }
};

// INHERITANCE: each concrete room reuses the Room interface and state.
class StandardRoom : public Room {
public:
    StandardRoom(int n) : Room(n) {}
    string getType() const override { return "Standard"; }
    double getPricePerNight() const override { return 1500.0; }
    string getAmenities() const override { return "Fan, TV"; }
};

class DeluxeRoom : public Room {
public:
    DeluxeRoom(int n) : Room(n) {}
    string getType() const override { return "Deluxe"; }
    double getPricePerNight() const override { return 2500.0; }
    string getAmenities() const override { return "AC, TV, Wi-Fi"; }
};

class SuiteRoom : public Room {
public:
    SuiteRoom(int n) : Room(n) {}
    string getType() const override { return "Suite"; }
    double getPricePerNight() const override { return 4500.0; }
    string getAmenities() const override { return "AC, TV, Wi-Fi, Mini-bar, Sofa"; }
};

// ======================= Customer =========================================
// ENCAPSULATION: customer data is private and exposed through getters.
class Customer {
private:
    string name, phone, idProof;
public:
    Customer() {}
    Customer(string n, string p, string id) : name(n), phone(p), idProof(id) {}
    string getName() const { return name; }
    string getPhone() const { return phone; }
    string getIdProof() const { return idProof; }
};

// ======================= Booking (with billing logic) =====================
// COMPOSITION: a Booking owns its Customer value.
// ENCAPSULATION: booking and billing data are private.
class Booking {
private:
    int bookingId;
    Customer customer;
    int roomNumber;
    int nights;
    string checkInDate;
    double rate;           // price per night
    double serviceCharges; // food, laundry, etc.

public:
    static constexpr double GST_RATE = 0.12;
    static constexpr double LONG_STAY_DISCOUNT = 0.10; // 5+ nights
    static const int LONG_STAY_NIGHTS = 5;

    Booking(int id, Customer c, int room, int n, string date, double r, double services = 0)
        : bookingId(id), customer(c), roomNumber(room), nights(n),
          checkInDate(date), rate(r), serviceCharges(services) {}

    int getId() const { return bookingId; }
    const Customer& getCustomer() const { return customer; }
    int getRoomNumber() const { return roomNumber; }
    int getNights() const { return nights; }
    string getCheckInDate() const { return checkInDate; }
    double getRate() const { return rate; }
    double getServiceCharges() const { return serviceCharges; }

    void extendStay(int extra) { nights += extra; }
    void addService(double amount) { serviceCharges += amount; }
    void changeRoom(int newRoom, double newRate) { roomNumber = newRoom; rate = newRate; }

    double roomCharge() const { return nights * rate; }
    double discount() const { return nights >= LONG_STAY_NIGHTS ? roomCharge() * LONG_STAY_DISCOUNT : 0.0; }
    double gst() const { return (roomCharge() + serviceCharges - discount()) * GST_RATE; }
    double total() const { return roomCharge() + serviceCharges - discount() + gst(); }
};

// ======================= Checkout history record ==========================
class HistoryRecord {
public:
    string name, phone, roomType, checkInDate;
    int roomNumber, nights, bookingId;
    double total;
    HistoryRecord(string n, string p, string t, string d, int r, int ni,
                  double tot, int id = 0)
        : name(n), phone(p), roomType(t), checkInDate(d), roomNumber(r),
          nights(ni), bookingId(id), total(tot) {}
};

// ======================= Hotel ============================================
class Hotel {
private:
    vector<unique_ptr<Room>> rooms;
    vector<Booking> bookings;
    vector<HistoryRecord> history;
    const string bookingFile = "bookings.txt";
    const string historyFile = "history.txt";

    struct Service { string name; double price; };
    vector<Service> services = {
        {"Food / Room service", 500}, {"Laundry", 200},
        {"Spa", 1500}, {"Airport pickup", 800}
    };

    Room* findRoom(int number) {
        for (auto& r : rooms) if (r->getNumber() == number) return r.get();
        return nullptr;
    }
    int findBookingIndex(int roomNumber) const {
        for (size_t i = 0; i < bookings.size(); i++)
            if (bookings[i].getRoomNumber() == roomNumber) return (int)i;
        return -1;
    }
    int nextBookingId() const {
        int mx = 1000;
        for (const auto& b : bookings) mx = max(mx, b.getId());
        // Include checked-out bookings so IDs are not reused after checkout.
        for (const auto& h : history) mx = max(mx, h.bookingId);
        return mx + 1;
    }
    void printRoomRow(const Room& r) const {
        cout << left << setw(8) << r.getNumber() << setw(12) << r.getType()
             << setw(12) << r.getPricePerNight() << setw(32) << r.getAmenities()
             << (r.isOccupied() ? "Occupied" : "Available") << "\n";
    }
    void printBookingRow(const Booking& b) const {
        cout << left << setw(7) << b.getId() << setw(7) << b.getRoomNumber()
             << setw(18) << b.getCustomer().getName() << setw(13) << b.getCustomer().getPhone()
             << setw(13) << b.getCheckInDate() << setw(8) << b.getNights()
             << fixed << setprecision(2) << b.total() << "\n";
    }
    void printBookingHeader() const {
        cout << left << setw(7) << "ID" << setw(7) << "Room" << setw(18) << "Customer"
             << setw(13) << "Phone" << setw(13) << "Check-in" << setw(8) << "Nights" << "Bill(Rs)\n";
    }
    void printBill(const Booking& b, const string& type) const {
        cout << fixed << setprecision(2);
        cout << "\n=========== BILL ===========\n"
             << "Booking ID : " << b.getId() << "\n"
             << "Customer   : " << b.getCustomer().getName() << "\n"
             << "Room       : " << b.getRoomNumber() << " (" << type << ")\n"
             << "Nights     : " << b.getNights() << " x Rs " << b.getRate() << "\n"
             << "Room charge: Rs " << b.roomCharge() << "\n"
             << "Services   : Rs " << b.getServiceCharges() << "\n"
             << "Discount   : -Rs " << b.discount() << (b.discount() > 0 ? "  (10% long-stay)" : "") << "\n"
             << "GST (12%)  : Rs " << b.gst() << "\n"
             << "----------------------------\n"
             << "TOTAL      : Rs " << b.total() << "\n"
             << "============================\n";
    }

public:
    Hotel() {
        for (int i = 101; i <= 104; i++) rooms.push_back(make_unique<StandardRoom>(i));
        for (int i = 201; i <= 204; i++) rooms.push_back(make_unique<DeluxeRoom>(i));
        for (int i = 301; i <= 302; i++) rooms.push_back(make_unique<SuiteRoom>(i));
        loadData();
    }

    // 1. All rooms with status
    void showAllRooms() const {
        cout << "\n--- All Rooms ---\n";
        cout << left << setw(8) << "Room" << setw(12) << "Type" << setw(12) << "Price/Night"
             << setw(32) << "Amenities" << "Status\n";
        for (const auto& r : rooms) printRoomRow(*r);
    }

    // 2. Available rooms
    void showAvailableRooms() const {
        cout << "\n--- Available Rooms ---\n";
        cout << left << setw(8) << "Room" << setw(12) << "Type" << setw(12) << "Price/Night"
             << setw(32) << "Amenities" << "Status\n";
        bool any = false;
        for (const auto& r : rooms)
            if (!r->isOccupied()) { printRoomRow(*r); any = true; }
        if (!any) cout << "No rooms available right now.\n";
    }

    // 3. Available rooms by type
    void showAvailableByType() const {
        cout << "\n1. Standard   2. Deluxe   3. Suite\n";
        int c = readInt("Choose room type: ");
        string type = c == 1 ? "Standard" : c == 2 ? "Deluxe" : c == 3 ? "Suite" : "";
        if (type.empty()) { cout << "Invalid type.\n"; return; }
        cout << "\n--- Available " << type << " Rooms ---\n";
        bool any = false;
        for (const auto& r : rooms)
            if (!r->isOccupied() && r->getType() == type) { printRoomRow(*r); any = true; }
        if (!any) cout << "No " << type << " rooms available.\n";
    }

    // 4. New booking
    void bookRoom() {
        showAvailableRooms();
        int number = readInt("\nEnter room number to book: ");
        Room* room = findRoom(number);
        if (!room) { cout << "Invalid room number.\n"; return; }
        if (room->isOccupied()) { cout << "Sorry, that room is already occupied.\n"; return; }

        string name = readLine("Customer name: ");
        if (name.find_first_not_of(" \t\r\n") == string::npos) {
            cout << "Name cannot be empty.\n";
            return;
        }
        string phone;
        do {
            phone = readLine("Phone number (10 digits): ");
            if (!validPhone(phone)) cout << "Invalid phone number.\n";
        } while (!validPhone(phone));
        string idProof = readLine("ID proof (Aadhaar/PAN): ");
        if (idProof.find_first_not_of(" \t\r\n") == string::npos) {
            cout << "ID proof cannot be empty.\n";
            return;
        }
        string date;
        do {
            date = readLine("Check-in date (DD-MM-YYYY): ");
            if (!validDate(date)) cout << "Invalid date format.\n";
        } while (!validDate(date));
        int nights = readInt("Number of nights: ");
        if (nights <= 0) { cout << "Nights must be at least 1.\n"; return; }

        bookings.emplace_back(nextBookingId(), Customer(name, phone, idProof),
                              number, nights, date, room->getPricePerNight());
        room->setOccupied(true);
        saveData();
        cout << "\nBooking confirmed! ID " << bookings.back().getId() << ", Room " << number
             << " (" << room->getType() << ") for " << name << ".\n";
    }

    // 5. Occupied rooms
    void showOccupiedRooms() const {
        cout << "\n--- Occupied Rooms & Customer Records ---\n";
        if (bookings.empty()) { cout << "No rooms are occupied.\n"; return; }
        printBookingHeader();
        for (const auto& b : bookings) printBookingRow(b);
    }

    // 6. Search by customer name
    void searchByName() const {
        string key = toLower(readLine("Enter customer name (or part of it): "));
        if (key.find_first_not_of(" \t\r\n") == string::npos) {
            cout << "Search name cannot be empty.\n";
            return;
        }
        bool found = false;
        for (const auto& b : bookings) {
            if (toLower(b.getCustomer().getName()).find(key) != string::npos) {
                if (!found) { cout << "\n"; printBookingHeader(); }
                printBookingRow(b);
                found = true;
            }
        }
        if (!found) cout << "No current guest matches that name.\n";
    }

    // Shows current guests, asks for a room, returns booking index (or -1)
    int selectActiveBooking(const string& title) const {
        cout << "\n--- " << title << " ---\n";
        if (bookings.empty()) {
            cout << "No guests are currently checked in.\n"
                 << "Please make a booking first (option 4).\n";
            return -1;
        }
        printBookingHeader();
        for (const auto& b : bookings) printBookingRow(b);
        int number = readInt("\nEnter room number: ");
        int idx = findBookingIndex(number);
        if (idx == -1) cout << "Room " << number << " has no active booking.\n";
        return idx;
    }

    // 7. Booking details by room number
    void viewBookingByRoom() const {
        int idx = selectActiveBooking("Booking Details");
        if (idx == -1) return;
        const Booking& b = bookings[idx];
        cout << fixed << setprecision(2)
             << "\nBooking ID : " << b.getId() << "\n"
             << "Name       : " << b.getCustomer().getName() << "\n"
             << "Phone      : " << b.getCustomer().getPhone() << "\n"
             << "ID proof   : " << b.getCustomer().getIdProof() << "\n"
             << "Room       : " << b.getRoomNumber() << "\n"
             << "Check-in   : " << b.getCheckInDate() << "\n"
             << "Nights     : " << b.getNights() << "\n"
             << "Services   : Rs " << b.getServiceCharges() << "\n"
             << "Bill so far: Rs " << b.total() << "\n";
    }

    // 8. Extend stay
    void extendStay() {
        int idx = selectActiveBooking("Extend Stay");
        if (idx == -1) return;
        int extra = readInt("Extra nights to add: ");
        if (extra <= 0) { cout << "Must be at least 1 night.\n"; return; }
        int before = bookings[idx].getNights();
        bookings[idx].extendStay(extra);
        saveData();
        cout << fixed << setprecision(2)
             << "\nStay extended for " << bookings[idx].getCustomer().getName() << ".\n"
             << "Nights: " << before << " -> " << bookings[idx].getNights() << "\n"
             << "Updated bill: Rs " << bookings[idx].total() << "\n";
    }

    // 9. Change room
    void changeRoom() {
        int idx = selectActiveBooking("Change Room");
        if (idx == -1) return;
        int oldNumber = bookings[idx].getRoomNumber();
        Room* oldRoom = findRoom(oldNumber);
        cout << "\nCurrent room: " << oldNumber << " (" << oldRoom->getType() << ")\n";
        showAvailableRooms();
        int newNumber = readInt("\nEnter new room number: ");
        Room* newRoom = findRoom(newNumber);
        if (!newRoom) { cout << "Invalid room number.\n"; return; }
        if (newRoom->isOccupied()) { cout << "That room is occupied.\n"; return; }
        oldRoom->setOccupied(false);
        newRoom->setOccupied(true);
        bookings[idx].changeRoom(newNumber, newRoom->getPricePerNight());
        saveData();
        cout << fixed << setprecision(2)
             << "\nRoom changed: " << oldNumber << " (" << oldRoom->getType() << ") -> "
             << newNumber << " (" << newRoom->getType() << ")\n"
             << "New rate: Rs " << newRoom->getPricePerNight() << " per night\n"
             << "Updated bill: Rs " << bookings[idx].total() << "\n";
    }

    // 10. Add service
    void addService() {
        int idx = selectActiveBooking("Add Service");
        if (idx == -1) return;
        cout << "\nAvailable services:\n";
        for (size_t i = 0; i < services.size(); i++)
            cout << "  " << i + 1 << ". " << services[i].name << " - Rs " << services[i].price << "\n";
        int c = readInt("Choose service: ");
        if (c < 1 || c > (int)services.size()) { cout << "Invalid service.\n"; return; }
        int qty = readInt("Quantity: ");
        if (qty <= 0) { cout << "Invalid quantity.\n"; return; }
        double cost = services[c - 1].price * qty;
        bookings[idx].addService(cost);
        saveData();
        cout << fixed << setprecision(2)
             << "\nAdded: " << services[c - 1].name << " x" << qty << " = Rs " << cost << "\n"
             << "Total services so far: Rs " << bookings[idx].getServiceCharges() << "\n"
             << "Updated bill: Rs " << bookings[idx].total() << "\n";
    }

    // 11. Vacate room
    void vacateRoom() {
        int idx = selectActiveBooking("Vacate Room & Generate Bill");
        if (idx == -1) return;
        Booking b = bookings[idx];
        int number = b.getRoomNumber();
        Room* room = findRoom(number);
        printBill(b, room->getType());
        history.emplace_back(b.getCustomer().getName(), b.getCustomer().getPhone(),
                             room->getType(), b.getCheckInDate(), number, b.getNights(),
                             b.total(), b.getId());
        room->setOccupied(false);
        bookings.erase(bookings.begin() + idx);
        saveData();
        cout << "\nRoom " << number << " is now vacant. Thank you for staying with us!\n";
    }

    // 12. Checkout history
    void showHistory() const {
        cout << "\n--- Checkout History ---\n";
        if (history.empty()) { cout << "No past guests yet.\n"; return; }
        cout << left << setw(8) << "ID" << setw(18) << "Customer" << setw(7) << "Room"
             << setw(11) << "Type" << setw(13) << "Check-in" << setw(8) << "Nights"
             << "Paid(Rs)\n";
        for (const auto& h : history)
            cout << left << setw(8) << (h.bookingId == 0 ? string("Legacy") : to_string(h.bookingId))
                 << setw(18) << h.name << setw(7) << h.roomNumber << setw(11) << h.roomType
                 << setw(13) << h.checkInDate << setw(8) << h.nights
                 << fixed << setprecision(2) << h.total << "\n";
    }

    // 13. Revenue / occupancy report
    void showReport() const {
        double collected = 0, expected = 0;
        for (const auto& h : history) collected += h.total;
        for (const auto& b : bookings) expected += b.total();
        int occ = (int)bookings.size(), total = (int)rooms.size();
        cout << fixed << setprecision(2)
             << "\n--- Hotel Report ---\n"
             << "Total rooms        : " << total << "\n"
             << "Occupied / Vacant  : " << occ << " / " << total - occ << "\n"
             << "Occupancy rate     : " << (100.0 * occ / total) << "%\n"
             << "Guests checked out : " << history.size() << "\n"
             << "Revenue collected  : Rs " << collected << "\n"
             << "Pending (current)  : Rs " << expected << "\n";
        for (string t : {"Standard", "Deluxe", "Suite"}) {
            double sum = 0; int cnt = 0;
            for (const auto& h : history) if (h.roomType == t) { sum += h.total; cnt++; }
            cout << "  " << left << setw(9) << t << ": " << cnt << " stays, Rs " << sum << "\n";
        }
    }

    // ---------- File handling ----------
    bool saveData() const {
        ofstream out(bookingFile);
        if (!out) {
            cerr << "\nWarning: Could not open " << bookingFile << " for writing.\n";
            return false;
        }
        out << fixed << setprecision(2);
        for (const auto& b : bookings) {
            out << b.getId() << "|" << b.getRoomNumber() << "|"
                << b.getCustomer().getName() << "|" << b.getCustomer().getPhone() << "|"
                << b.getCustomer().getIdProof() << "|" << b.getCheckInDate() << "|"
                << b.getNights() << "|" << b.getRate() << "|"
                << b.getServiceCharges() << "\n";
        }
        out.close();
        if (!out) {
            cerr << "\nWarning: Failed while saving " << bookingFile << ".\n";
            return false;
        }

        ofstream hout(historyFile);
        if (!hout) {
            cerr << "\nWarning: Could not open " << historyFile << " for writing.\n";
            return false;
        }
        hout << fixed << setprecision(2);
        for (const auto& h : history) {
            hout << h.name << "|" << h.phone << "|" << h.roomType << "|"
                 << h.checkInDate << "|" << h.roomNumber << "|" << h.nights << "|"
                 << h.total << "|" << h.bookingId << "\n";
        }
        hout.close();
        if (!hout) {
            cerr << "\nWarning: Failed while saving " << historyFile << ".\n";
            return false;
        }
        return true;
    }

    void loadData() {
        string line;
        ifstream in(bookingFile);
        while (in && getline(in, line)) {
            stringstream ss(line);
            string f[9];
            for (int i = 0; i < 9; ++i) {
                if (!getline(ss, f[i], '|')) f[i].clear();
            }
            if (f[0].empty() || f[8].empty()) continue;
            try {
                size_t p0, p1, p6, p7, p8;
                int id = stoi(f[0], &p0);
                int roomNumber = stoi(f[1], &p1);
                int nights = stoi(f[6], &p6);
                double rate = stod(f[7], &p7);
                double serviceCharges = stod(f[8], &p8);
                if (p0 != f[0].size() || p1 != f[1].size() ||
                    p6 != f[6].size() || p7 != f[7].size() ||
                    p8 != f[8].size() || id <= 0 || nights <= 0 ||
                    rate < 0 || serviceCharges < 0 || !validDate(f[5]) ||
                    !validPhone(f[3]) || f[2].find_first_not_of(" \t\r\n") == string::npos ||
                    findBookingIndex(roomNumber) != -1) {
                    continue;
                }
                Room* room = findRoom(roomNumber);
                if (!room || room->isOccupied()) continue;
                bool duplicateId = false;
                for (const auto& b : bookings) {
                    if (b.getId() == id) { duplicateId = true; break; }
                }
                if (duplicateId) continue;

                // Mark occupied only after the complete record is validated and loaded.
                bookings.emplace_back(id, Customer(f[2], f[3], f[4]), roomNumber,
                                      nights, f[5], rate, serviceCharges);
                room->setOccupied(true);
            } catch (...) {
                continue; // Ignore malformed records instead of corrupting room status.
            }
        }

        ifstream hin(historyFile);
        while (hin && getline(hin, line)) {
            stringstream ss(line);
            string f[8];
            for (int i = 0; i < 8; ++i) {
                if (!getline(ss, f[i], '|')) f[i].clear();
            }
            if (f[6].empty()) continue;
            try {
                size_t p4, p5, p6;
                int roomNumber = stoi(f[4], &p4);
                int nights = stoi(f[5], &p5);
                double totalAmount = stod(f[6], &p6);
                if (p4 != f[4].size() || p5 != f[5].size() ||
                    p6 != f[6].size() || roomNumber <= 0 || nights <= 0 || totalAmount < 0) {
                    continue;
                }
                // Old history files had 7 fields. The new format appends booking ID.
                int id = f[7].empty() ? 0 : stoi(f[7]);
                if (id < 0) continue;
                history.emplace_back(f[0], f[1], f[2], f[3], roomNumber,
                                     nights, totalAmount, id);
            } catch (...) {
                continue;
            }
        }
    }
};

// ======================= Main menu ========================================
void showMenu() {
    cout << "\n=========== HOTEL RESERVATION SYSTEM ===========\n"
         << " -- Rooms --\n"
         << "  1. View all rooms (with status)\n"
         << "  2. Check available rooms\n"
         << "  3. Available rooms by type\n"
         << " -- Bookings --\n"
         << "  4. New booking\n"
         << "  5. Occupied rooms & customer records\n"
         << "  6. Search customer by name\n"
         << "  7. View booking details by room number\n"
         << " -- Guest services --\n"
         << "  8. Extend stay\n"
         << "  9. Change room\n"
         << " 10. Add service (food, laundry, spa...)\n"
         << " 11. Vacate room & generate bill\n"
         << " -- Reports --\n"
         << " 12. Checkout history\n"
         << " 13. Revenue & occupancy report\n"
         << " 14. Exit\n";
}

void showWelcome() {
    time_t now = time(nullptr);
    int hour = localtime(&now)->tm_hour;
    string greeting = hour < 12 ? "Good morning" : hour < 17 ? "Good afternoon" : "Good evening";
    cout << "\n**************************************************\n"
         << "*                                                *\n"
         << "*        WELCOME TO OUR HOTEL                    *\n"
         << "*        Your comfort is our priority            *\n"
         << "*                                                *\n"
         << "**************************************************\n"
         << "\n" << greeting << "! How may we help you today?\n";
}

int main() {
    try {
        showWelcome();
        Hotel hotel;
        int choice;
        do {
            showMenu();
            choice = readInt("Enter choice: ");
            switch (choice) {
                case 1:  hotel.showAllRooms(); break;
                case 2:  hotel.showAvailableRooms(); break;
                case 3:  hotel.showAvailableByType(); break;
                case 4:  hotel.bookRoom(); break;
                case 5:  hotel.showOccupiedRooms(); break;
                case 6:  hotel.searchByName(); break;
                case 7:  hotel.viewBookingByRoom(); break;
                case 8:  hotel.extendStay(); break;
                case 9:  hotel.changeRoom(); break;
                case 10: hotel.addService(); break;
                case 11: hotel.vacateRoom(); break;
                case 12: hotel.showHistory(); break;
                case 13: hotel.showReport(); break;
                case 14: cout << "\nThank you for choosing our hotel. Have a wonderful day!\n"; break;
                default: cout << "Invalid choice, try again.\n";
            }
        } while (choice != 14);
    } catch (const runtime_error& error) {
        cout << "\nProgram stopped safely: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
