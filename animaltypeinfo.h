#ifndef ANIMALTYPEINFO_H
#define ANIMALTYPEINFO_H

#include <QString>
#include <QMap>
#include <QSqlQuery>

//-------------------------------------------------------------------------
// AnimalTypeInfo
// Reads the animal type information from the database once and caches it
// Usage: auto info = AnimalTypeInfo::get(animalTypeId);
//-------------------------------------------------------------------------

struct AnimalTypeInfo {
    int     id      = -1;
    QString name;       // "سگ"
    QString emoji;      // "🐕"
    QString badgeBg;    // "#E8F5E9"
    QString badgeFg;    // "#1B5E20"
    bool    valid   = false;

    // Get information based on animal_type_id
    static AnimalTypeInfo get(int typeId)
    {
        return loadCache().value(typeId, AnimalTypeInfo{});
    }

    // Clear cache — Call after database changes
    static void clearCache()
    {
        loadCache(true);
    }

private:
    static QMap<int, AnimalTypeInfo>& loadCache(bool forceReload = false)
    {
        static QMap<int, AnimalTypeInfo> cache;
        if (!cache.isEmpty() && !forceReload)
            return cache;

        cache.clear();
        QSqlQuery q;
        q.exec("SELECT id, name, emoji, badge_bg, badge_fg FROM animal_types ORDER BY id");
        while (q.next()) {
            AnimalTypeInfo info;
            info.id      = q.value("id").toInt();
            info.name    = q.value("name").toString();
            info.emoji   = q.value("emoji").toString();
            info.badgeBg = q.value("badge_bg").toString();
            info.badgeFg = q.value("badge_fg").toString();
            info.valid   = true;
            cache[info.id] = info;
        }
        return cache;
    }
};

#endif // ANIMALTYPEINFO_H