#ifndef PAGEDIRTYTRACKER_H
#define PAGEDIRTYTRACKER_H

#include <QSet>
#include <initializer_list>

// ─────────────────────────────────────────────────────────────────────────────
// PageDirtyTracker — Singleton that only remembers "which page was last visited, data indicating that it has changed somewhere else".
//
// It emits no signals and no widgets are connected to it — just two actions:
// markDirty({...}) → It will sound successful after every add/edit/delete
// consumeDirty(page) → MainWindow calls when the user enters a page;
// If it returns true, then reloadPreservingState()
// that page should be called. It also clears the flag so that
// it doesn't reload again for no reason the next time.
// ─────────────────────────────────────────────────────────────────────────────

enum class AppPage {
    Dashboard,
    Animals,
    Owners,
    Reminders,
    Vaccinations,
    AnimalTypes
};

class PageDirtyTracker
{
public:
    static PageDirtyTracker& instance()
    {
        static PageDirtyTracker t;
        return t;
    }

    void markDirty(std::initializer_list<AppPage> pages)
    {
        for (AppPage p : pages)
            m_dirty.insert(p);
    }

    void markDirty(AppPage page)
    {
        m_dirty.insert(page);
    }

    // اگر صفحه کثیف بود true برمی‌گرداند و فلگش را پاک می‌کند؛ وگرنه false
    bool consumeDirty(AppPage page)
    {
        return m_dirty.remove(page) > 0;
    }

private:
    PageDirtyTracker() = default;
    PageDirtyTracker(const PageDirtyTracker&) = delete;
    PageDirtyTracker& operator=(const PageDirtyTracker&) = delete;

    QSet<AppPage> m_dirty;
};

#endif // PAGEDIRTYTRACKER_H