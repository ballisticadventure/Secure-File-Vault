# Secure-File-Vault
Secure File Vault is a C++ desktop application that provides secure file encryption and decryption using AES-256 encryption with OpenSSL. The project features a GTK-based graphical user interface, user authentication, real-time progress tracking, and file information monitoring, enabling users to protect sensitive files through a simple interface.

# Features
* AES-256-CBC file encryption and decryption
* User authentication system
* Modern GTK3 graphical user interface
* Password-protected file operations
* Real-time progress bar
* File information display
* Encrypted file detection

# Technologies Used
* C++
* GTK+3
* OpenSSL
* AES-256 Encryption
* File Handling

# Requirements

Ensure the following dependencies are installed:

* C++17 compatible compiler (GCC/G++)
* GTK+3 Development Libraries
* OpenSSL Development Libraries
* MSYS2 (for Windows users)

# How To Run
* Compile the code:- g++ securefilevault.cpp -o SecureVault.exe $(pkg-config --cflags --libs gtk+-3.0) -lssl -lcrypto
* Run the exe program:- ./SecureVault.exe

# Login Credentials
* Username: admin
* Password: 1234
# Note:- These credentials are for demonstration purposes only
