#ifndef BACKUPTAB_H
#define BACKUPTAB_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QProcess>

// ─────────────────────────────────────────────────────────────────────────────
// BackupTab — Admin-only backup settings tab
// ─────────────────────────────────────────────────────────────────────────────

class BackupTab : public QWidget
{
    Q_OBJECT

public:
    explicit BackupTab(QWidget* parent = nullptr);

    // Called on app startup to check and run auto-backup if due
    static void checkAndRunAutoBackup();

private slots:
    void onBrowsePath();
    void onSaveSettings();
    void onBackupNow();

private:
    void applyStyle();
    void loadSettings();

    static bool runBackup(const QString& folder,
                          const QString& password,
                          QString& errorOut);

    QLineEdit*   m_pathEdit            = nullptr;
    QSpinBox*    m_intervalSpin        = nullptr;
    QLineEdit*   m_passwordEdit        = nullptr;
    QPushButton* m_btnShowPassword     = nullptr;
    QPushButton* m_btnChangePassword   = nullptr;
    QLabel*      m_passwordStatusLbl   = nullptr;
    QLabel*      m_lastBackupLabel     = nullptr;
    QPushButton* m_btnBrowse           = nullptr;
    QPushButton* m_btnSave             = nullptr;
    QPushButton* m_btnBackupNow        = nullptr;
    bool         m_passwordIsSet       = false;
};

#endif // BACKUPTAB_H