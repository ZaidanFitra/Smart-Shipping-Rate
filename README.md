# Smart Shipping Rate

## About
Smart Shipping Rate is a system that can calculate total shipping costs automatically based on volume of goods, mass of goods, and shipping distance.
- The volume of the goods is measured using 3 ultrasonic sensors with different axes.
- The mass of the goods is measured using a load cell sensor.
- The shipping distance is input by the user on the mobile application.
- Data is stored in a real time database on **Google Firebase**.
- Determination of shipping rates is done on the ESP32 using a **Fuzzy Logic** algorithm.
- Total shipping cost is displayed on the mobile application.

## Block Diagram of the System
![Block Diagram](https://github.com/ZaidanFitra/Smart-Shipping-Rate/blob/main/Block-Diagram-of-the-System.png?raw=true)
