#ifndef SESSION_H
#define SESSION_H

#include <QString>

// ─────────────────────────────────────────────────────────────────────────────
// Session — Singleton that holds the current logged-in user's info
// Set once after login, read from any widget without passing role around
// Usage:
//   Session::instance().setUser(id, username, role);
//   if (Session::instance().isAdmin()) { ... }
// ─────────────────────────────────────────────────────────────────────────────

class Session
{
public:
    static Session& instance() {
        static Session s;
        return s;
    }

    void setUser(int id, const QString& username, const QString& role) {
        m_userId   = id;
        m_username = username;
        m_role     = role;
    }

    int     userId()   const { return m_userId; }
    QString username() const { return m_username; }
    QString role()     const { return m_role; }
    bool    isAdmin()  const { return m_role == "admin"; }

    void clear() {
        m_userId   = -1;
        m_username.clear();
        m_role.clear();
    }

private:
    Session() = default;
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    int     m_userId   = -1;
    QString m_username;
    QString m_role;
};

#endif // SESSION_H