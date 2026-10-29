# Hotel Reservation System

## About the Project

This is a console-based hotel management project built in C++ to make room booking, guest handling and billing simple and organised. The system lets a hotel receptionist check room availability, book rooms, manage guest stays, add services, generate bills and view reports, all from one menu-driven program.

I created this project for my CSE 202 (Object-Oriented Programming) course to apply C++ and OOP concepts to a real-world problem and to understand how a booking and billing system works behind the scenes.

## Features

- View all rooms with live status (Available / Occupied)
- Check available rooms, or filter them by type (Standard, Deluxe, Suite)
- New booking with customer name, phone number, ID proof, check-in date and number of nights
- Automatic booking ID generation (IDs are never reused after checkout)
- Search current guests by name (full or partial, not case-sensitive)
- View booking details by room number
- Extend a guest's stay
- Change a guest's room (the rate and bill update automatically)
- Add services: Food / Room service, Laundry, Spa, Airport pickup
- Vacate a room and generate a detailed bill
- Automatic billing: 10% long-stay discount for 5 or more nights, plus 12% GST
- Checkout history of past guests
- Revenue and occupancy report (collected revenue, pending bills, occupancy rate, stays per room type)
- Data is saved to files, so bookings and history remain after the program is closed
- Input validation for phone numbers (10 digits), real calendar dates (including leap years), empty fields and invalid menu choices

## Room Types and Prices

| Type | Rooms | Price per Night | Amenities |
|---|---|---|---|
| Standard | 101 - 104 | Rs 1500 | Fan, TV |
| Deluxe | 201 - 204 | Rs 2500 | AC, TV, Wi-Fi |
| Suite | 301 - 302 | Rs 4500 | AC, TV, Wi-Fi, Mini-bar, Sofa |

## Sample Bill

```
=========== BILL ===========
Booking ID : 1001
Customer   : Rahul
Room       : 101 (Standard)
Nights     : 6 x Rs 1500.00
Room charge: Rs 9000.00
Services   : Rs 1500.00
Discount   : -Rs 900.00  (10% long-stay)
GST (12%)  : Rs 1152.00
----------------------------
TOTAL      : Rs 10752.00
============================
```

## Technologies Used

- Language: C++ (C++14 or newer, tested with C++17)
- Standard Template Library: vector, string, algorithm, iomanip, fstream, sstream
- Smart pointers (`unique_ptr`) for memory management
- Object-Oriented Programming (OOP)
- File Handling

## OOP Concepts Used

- **Encapsulation:** Classes like `Room`, `Customer`, `Booking` and `Hotel` keep their data private and expose it only through public methods.
- **Abstraction:** `Room` is an abstract class that defines the common interface (`getType`, `getPricePerNight`, `getAmenities`) for every room type.
- **Inheritance:** `StandardRoom`, `DeluxeRoom` and `SuiteRoom` inherit from `Room` and reuse its state and interface.
- **Polymorphism:** Virtual functions are overridden by each room type, so the hotel handles all rooms through `Room` pointers and each one gives its own type, price and amenities.
- **Composition:** A `Booking` owns a `Customer` object, and the `Hotel` owns its rooms, bookings and history.

## File Handling

- `bookings.txt` stores current guests.
- `history.txt` stores checkout history.
- Records use `|` as a separator, and the program skips malformed or duplicate records when loading so the data stays consistent.
- Files are created automatically the first time you make a booking.

## How to Run

1. Make sure a C++ compiler (g++) is installed.
2. Compile:
   ```
   g++ -std=c++17 hotel.cpp -o hotel
   ```
3. Run:
   ```
   ./hotel
   ```
   (On Windows, run `hotel.exe`.)
4. Use the menu by entering the number of the option you want.

## Menu Options

| No. | Option |
|---|---|
| 1 | View all rooms (with status) |
| 2 | Check available rooms |
| 3 | Available rooms by type |
| 4 | New booking |
| 5 | Occupied rooms and customer records |
| 6 | Search customer by name |
| 7 | View booking details by room number |
| 8 | Extend stay |
| 9 | Change room |
| 10 | Add service (food, laundry, spa, airport pickup) |
| 11 | Vacate room and generate bill |
| 12 | Checkout history |
| 13 | Revenue and occupancy report |
| 14 | Exit |

## What I Learned

While working on this project, I got hands-on experience with designing classes and class hierarchies, using abstract classes and virtual functions, managing objects with smart pointers and STL vectors, writing billing logic, and reading and writing data with file handling. I also learned how to validate user input and build a complete, menu-driven application that solves a real-life problem.
