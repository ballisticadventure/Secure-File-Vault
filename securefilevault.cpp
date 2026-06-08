#include <gtk/gtk.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <sys/stat.h>

using namespace std;

// ================= GLOBAL VARIABLES =================

GtkWidget *fileChooser;
GtkWidget *passwordEntry;
GtkWidget *statusLabel;
GtkWidget *progressBar;
GtkWidget *fileInfoLabel;

GtkWidget *mainWindow;
GtkWidget *loginWindow;

GtkWidget *usernameEntry;
GtkWidget *loginPasswordEntry;

// ================= FILE INFO =================

void updateFileInfo(const char *filepath)
{
    struct stat fileStat;

    if (stat(filepath, &fileStat) == 0)
    {
        string filename = filepath;

        string extension = "Unknown";

        size_t dotPos = filename.find_last_of(".");

        if (dotPos != string::npos)
        {
            extension = filename.substr(dotPos + 1);
        }

        string status =
            (extension == "enc") ? "Encrypted" : "Normal";

        string info =
            "📄 File: " + filename +
            "\n📦 Size: " + to_string(fileStat.st_size / 1024.0) + " KB" +
            "\n🧩 Extension: ." + extension +
            "\n🔐 Status: " + status;

        gtk_label_set_text(GTK_LABEL(fileInfoLabel),
                           info.c_str());
    }
}

// ================= KEY GENERATION =================

void generateKey(const string &password,
                 unsigned char *key)
{
    memset(key, 0, 32);

    for (size_t i = 0;
         i < password.size() && i < 32;
         i++)
    {
        key[i] = password[i];
    }
}

// ================= PROGRESS BAR =================

void updateProgress(double value)
{
    gtk_progress_bar_set_fraction(
        GTK_PROGRESS_BAR(progressBar),
        value);

    while (gtk_events_pending())
    {
        gtk_main_iteration();
    }
}

// ================= ENCRYPT =================

void encryptFile(string inputFile,
                 string password)
{
    FILE *in = fopen(inputFile.c_str(), "rb");

    if (!in)
    {
        gtk_label_set_text(GTK_LABEL(statusLabel),
                           "❌ Cannot open input file!");
        return;
    }

    string outputFile = inputFile + ".enc";

    FILE *out = fopen(outputFile.c_str(), "wb");

    if (!out)
    {
        fclose(in);

        gtk_label_set_text(GTK_LABEL(statusLabel),
                           "❌ Cannot create encrypted file!");

        return;
    }

    unsigned char key[32];
    unsigned char iv[16];

    generateKey(password, key);

    memset(iv, 0, 16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx,
                       EVP_aes_256_cbc(),
                       NULL,
                       key,
                       iv);

    fseek(in, 0, SEEK_END);

    long filesize = ftell(in);

    rewind(in);

    unsigned char inbuf[1024];
    unsigned char outbuf[1040];

    int inlen;
    int outlen;

    long processed = 0;

    while ((inlen = fread(inbuf,
                          1,
                          1024,
                          in)) > 0)
    {
        EVP_EncryptUpdate(ctx,
                          outbuf,
                          &outlen,
                          inbuf,
                          inlen);

        fwrite(outbuf,
               1,
               outlen,
               out);

        processed += inlen;

        updateProgress((double)processed / filesize);
    }

    EVP_EncryptFinal_ex(ctx,
                        outbuf,
                        &outlen);

    fwrite(outbuf,
           1,
           outlen,
           out);

    fclose(in);
    fclose(out);

    EVP_CIPHER_CTX_free(ctx);

    updateProgress(1.0);

    gtk_label_set_text(GTK_LABEL(statusLabel),
                       "✅ Encryption Successful!");
}

// ================= DECRYPT =================

void decryptFile(string inputFile,
                 string password)
{
    FILE *in = fopen(inputFile.c_str(), "rb");

    if (!in)
    {
        gtk_label_set_text(GTK_LABEL(statusLabel),
                           "❌ Cannot open encrypted file!");

        return;
    }

    string outputFile;

    if (inputFile.find(".enc") != string::npos)
    {
        outputFile =
            inputFile.substr(0,
                             inputFile.size() - 4);
    }
    else
    {
        outputFile = "decrypted.txt";
    }

    FILE *out = fopen(outputFile.c_str(), "wb");

    unsigned char key[32];
    unsigned char iv[16];

    generateKey(password, key);

    memset(iv, 0, 16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx,
                       EVP_aes_256_cbc(),
                       NULL,
                       key,
                       iv);

    fseek(in, 0, SEEK_END);

    long filesize = ftell(in);

    rewind(in);

    unsigned char inbuf[1024];
    unsigned char outbuf[1040];

    int inlen;
    int outlen;

    long processed = 0;

    bool success = true;

    while ((inlen = fread(inbuf,
                          1,
                          1024,
                          in)) > 0)
    {
        if (!EVP_DecryptUpdate(ctx,
                               outbuf,
                               &outlen,
                               inbuf,
                               inlen))
        {
            success = false;
            break;
        }

        fwrite(outbuf,
               1,
               outlen,
               out);

        processed += inlen;

        updateProgress((double)processed / filesize);
    }

    if (success)
    {
        if (!EVP_DecryptFinal_ex(ctx,
                                 outbuf,
                                 &outlen))
        {
            success = false;
        }
        else
        {
            fwrite(outbuf,
                   1,
                   outlen,
                   out);
        }
    }

    fclose(in);
    fclose(out);

    EVP_CIPHER_CTX_free(ctx);

    updateProgress(1.0);

    if (!success)
    {
        remove(outputFile.c_str());

        gtk_label_set_text(GTK_LABEL(statusLabel),
                           "❌ Wrong Password!");
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(statusLabel),
                           "✅ Decryption Successful!");
    }
}

// ================= BUTTON EVENTS =================

void on_encrypt_clicked(GtkWidget *widget,
                        gpointer data)
{
    char *filename =
        gtk_file_chooser_get_filename(
            GTK_FILE_CHOOSER(fileChooser));

    const char *password =
        gtk_entry_get_text(GTK_ENTRY(passwordEntry));

    if (filename && strlen(password) > 0)
    {
        updateFileInfo(filename);

        updateProgress(0);

        encryptFile(filename, password);
    }
}

void on_decrypt_clicked(GtkWidget *widget,
                        gpointer data)
{
    char *filename =
        gtk_file_chooser_get_filename(
            GTK_FILE_CHOOSER(fileChooser));

    const char *password =
        gtk_entry_get_text(GTK_ENTRY(passwordEntry));

    if (filename && strlen(password) > 0)
    {
        updateFileInfo(filename);

        updateProgress(0);

        decryptFile(filename, password);
    }
}

// ================= LOGIN =================

void on_login_clicked(GtkWidget *widget,
                      gpointer data)
{
    const char *username =
        gtk_entry_get_text(GTK_ENTRY(usernameEntry));

    const char *password =
        gtk_entry_get_text(GTK_ENTRY(loginPasswordEntry));

    if (strcmp(username, "admin") == 0 &&
        strcmp(password, "1234") == 0)
    {
        gtk_widget_hide(loginWindow);

        gtk_widget_show_all(mainWindow);
    }
    else
    {
        GtkWidget *dialog;

        dialog = gtk_message_dialog_new(
            GTK_WINDOW(loginWindow),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Invalid Username or Password!");

        gtk_window_set_title(GTK_WINDOW(dialog),
                             "Login Failed");

        gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);
    }
}

// ================= MAIN =================

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    // ================= CSS THEME =================

    GtkCssProvider *provider =
        gtk_css_provider_new();

    gtk_css_provider_load_from_data(
        provider,

        "window {"
        "   background: #1E1E2F;"
        "}"

        "label {"
        "   color: #FFFFFF;"
        "   font-size: 15px;"
        "   font-weight: bold;"
        "}"

        "messagedialog label {"
        "   color: black;"
        "}"

        "entry {"
        "   background: #2D2D44;"
        "   color: white;"
        "   border-radius: 10px;"
        "   padding: 10px;"
        "   border: 2px solid #6C63FF;"
        "}"

        "button {"
        "   background: #6C63FF;"
        "   color: white;"
        "   border-radius: 12px;"
        "   padding: 12px;"
        "   font-size: 15px;"
        "   font-weight: bold;"
        "}"

        "button:hover {"
        "   background: #8B80FF;"
        "}"

        "progressbar trough {"
        "   background: #2D2D44;"
        "   border-radius: 10px;"
        "   min-height: 20px;"
        "}"

        "progressbar progress {"
        "   background: #00D1B2;"
        "   border-radius: 10px;"
        "}"

        "#titleLabel {"
        "   color: #00D1B2;"
        "   font-size: 30px;"
        "   font-weight: bold;"
        "}"

        "#infoPanel {"
        "   background: #2D2D44;"
        "   border-radius: 12px;"
        "   padding: 15px;"
        "}"

        "#statusPanel {"
        "   color: #00FF99;"
        "   font-weight: bold;"
        "}",

        -1,
        NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    // ================= LOGIN WINDOW =================

    loginWindow =
        gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(loginWindow),
                         "Vault Login");

    gtk_window_set_default_size(GTK_WINDOW(loginWindow),
                                400,
                                250);

    gtk_window_set_position(GTK_WINDOW(loginWindow),
                            GTK_WIN_POS_CENTER);

    GtkWidget *loginBox =
        gtk_box_new(GTK_ORIENTATION_VERTICAL,
                    15);

    gtk_container_set_border_width(
        GTK_CONTAINER(loginBox),
        25);

    gtk_container_add(GTK_CONTAINER(loginWindow),
                      loginBox);

    GtkWidget *loginTitle =
        gtk_label_new("🔐 Secure Vault");

    gtk_widget_set_name(loginTitle,
                        "titleLabel");

    gtk_box_pack_start(GTK_BOX(loginBox),
                       loginTitle,
                       FALSE,
                       FALSE,
                       10);

    usernameEntry = gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(usernameEntry),
        "Username");

    gtk_box_pack_start(GTK_BOX(loginBox),
                       usernameEntry,
                       FALSE,
                       FALSE,
                       5);

    loginPasswordEntry = gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(loginPasswordEntry),
        "Password");

    gtk_entry_set_visibility(
        GTK_ENTRY(loginPasswordEntry),
        FALSE);

    gtk_box_pack_start(GTK_BOX(loginBox),
                       loginPasswordEntry,
                       FALSE,
                       FALSE,
                       5);

    GtkWidget *loginBtn =
        gtk_button_new_with_label("LOGIN");

    gtk_box_pack_start(GTK_BOX(loginBox),
                       loginBtn,
                       FALSE,
                       FALSE,
                       10);

    g_signal_connect(loginBtn,
                     "clicked",
                     G_CALLBACK(on_login_clicked),
                     NULL);

    // ================= MAIN WINDOW =================

    mainWindow =
        gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(mainWindow),
                         "Secure File Vault");

    gtk_window_set_default_size(GTK_WINDOW(mainWindow),
                                700,
                                550);

    gtk_window_set_position(GTK_WINDOW(mainWindow),
                            GTK_WIN_POS_CENTER);

    g_signal_connect(mainWindow,
                     "destroy",
                     G_CALLBACK(gtk_main_quit),
                     NULL);

    GtkWidget *mainBox =
        gtk_box_new(GTK_ORIENTATION_VERTICAL,
                    20);

    gtk_container_set_border_width(
        GTK_CONTAINER(mainBox),
        25);

    gtk_container_add(GTK_CONTAINER(mainWindow),
                      mainBox);

    GtkWidget *title =
        gtk_label_new("🔐 Secure File Vault");

    gtk_widget_set_name(title,
                        "titleLabel");

    gtk_box_pack_start(GTK_BOX(mainBox),
                       title,
                       FALSE,
                       FALSE,
                       5);

    fileChooser =
        gtk_file_chooser_button_new(
            "Choose File",
            GTK_FILE_CHOOSER_ACTION_OPEN);

    gtk_box_pack_start(GTK_BOX(mainBox),
                       fileChooser,
                       FALSE,
                       FALSE,
                       5);

    passwordEntry = gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(passwordEntry),
        "Enter Encryption Password");

    gtk_entry_set_visibility(
        GTK_ENTRY(passwordEntry),
        FALSE);

    gtk_box_pack_start(GTK_BOX(mainBox),
                       passwordEntry,
                       FALSE,
                       FALSE,
                       5);

    GtkWidget *encryptBtn =
        gtk_button_new_with_label("🔒 Encrypt File");

    GtkWidget *decryptBtn =
        gtk_button_new_with_label("🔓 Decrypt File");

    gtk_box_pack_start(GTK_BOX(mainBox),
                       encryptBtn,
                       FALSE,
                       FALSE,
                       5);

    gtk_box_pack_start(GTK_BOX(mainBox),
                       decryptBtn,
                       FALSE,
                       FALSE,
                       5);

    progressBar =
        gtk_progress_bar_new();

    gtk_box_pack_start(GTK_BOX(mainBox),
                       progressBar,
                       FALSE,
                       FALSE,
                       10);

    fileInfoLabel =
        gtk_label_new("📄 No file selected");

    gtk_widget_set_name(fileInfoLabel,
                        "infoPanel");

    gtk_box_pack_start(GTK_BOX(mainBox),
                       fileInfoLabel,
                       FALSE,
                       FALSE,
                       10);

    statusLabel =
        gtk_label_new("🟢 System Ready");

    gtk_widget_set_name(statusLabel,
                        "statusPanel");

    gtk_box_pack_start(GTK_BOX(mainBox),
                       statusLabel,
                       FALSE,
                       FALSE,
                       10);

    // ================= SIGNALS =================

    g_signal_connect(encryptBtn,
                     "clicked",
                     G_CALLBACK(on_encrypt_clicked),
                     NULL);

    g_signal_connect(decryptBtn,
                     "clicked",
                     G_CALLBACK(on_decrypt_clicked),
                     NULL);

    // ================= START =================

    gtk_widget_show_all(loginWindow);

    gtk_main();

    return 0;
}