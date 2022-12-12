#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <type_traits>

using namespace std;

enum ItemCategory
{
    IC_STATIONARY,
    IC_MACHINERY,
    IC_ACCESSORY,
};

struct MemberData
{
    char name[50];
    int borrow_count = 0;
};

struct Member
{
    // std::string name;
    MemberData data;

    Member* prev = nullptr;
    Member* next = nullptr;
};

typedef uint16_t item_id_t;

struct ItemMeta
{
    // std::string name {};
    char name[50];
    ItemCategory cat;

    // ItemMeta(ItemMeta&& other) = default;
};

struct InventoryItem
{
    item_id_t item_id = 0;
    ItemMeta meta {};
    uint32_t item_count = 0;
    Member* allocated_to = nullptr;
    bool active = true;
};

// auto dwajdaw3 = std::is_trivially_copyable<InventoryItem>::value;
// auto dwajdaw = sizeof(InventoryItem);

struct Inventory
{
    InventoryItem* items;
    uint32_t count;
    uint32_t capacity;
    item_id_t next_id;
};

inline constexpr uint32_t grow(uint32_t old)
{
    return old < 8 ? 8 : 2 * old;
}

inline Member* CreateMember(const std::string& name)
{
    return new Member { name, 0, nullptr, nullptr };
}

inline void DeleteMember(Member* m)
{
    delete m;
}

InventoryItem* InvUtil_FindItemById(Inventory& inv, item_id_t id, bool active_only = true)
{
    for (int i = 0; i < inv.count; ++i)
    {
        auto& item = inv.items[i];
        if (item.item_id == id && (!active_only || item.active))
            return &item;
    }

    return nullptr;
}

enum class InvResult
{
    ErrUnknown,
    ErrNotFound,
    ErrOutOfMemory,
    ErrInvalidPayload,
    ErrDuplicateEntry,
    WarnEmpty,
    Ok,
};

InvResult Inv_AddItem(Inventory& inv, item_id_t id, ItemMeta&& meta);
InvResult Inv_ViewItems(Inventory& inv);
InvResult Inv_SearchItem(Inventory& inv, const std::string& str);
InvResult Inv_EditItem(Inventory& inv, item_id_t id, ItemMeta&& item);
InvResult Inv_DeleteItem(Inventory& inv, item_id_t id);
InvResult Inv_AssignItem(Inventory& inv, item_id_t id, const std::string& to);
InvResult Inv_RetrieveItem(Inventory& inv, item_id_t id, const std::string& from);
InvResult Inv_ItemDetails(Inventory& inv, item_id_t id);

int main()
{
    Inventory inv;

    inv.count = 0;
    inv.capacity = 1 << 7;
    inv.items = new InventoryItem[inv.capacity];

    Inv_AddItem(inv,  1, { "Heheh 1", IC_ACCESSORY });
    Inv_AddItem(inv,  2, { "Heheh 2", IC_ACCESSORY });
    Inv_AddItem(inv,  3, { "Heheh 3", IC_ACCESSORY });
    Inv_AddItem(inv,  4, { "Heheh 4", IC_ACCESSORY });
    Inv_AddItem(inv,  5, { "Heheh 5", IC_ACCESSORY });
    Inv_AddItem(inv,  6, { "Heheh 6", IC_ACCESSORY });
    Inv_AddItem(inv,  7, { "Heheh 7", IC_ACCESSORY });
    Inv_AddItem(inv,  8, { "Heheh 8", IC_ACCESSORY });
    Inv_AddItem(inv,  9, { "Heheh 9", IC_ACCESSORY });
    Inv_AddItem(inv, 10, { "Heheh 10", IC_ACCESSORY });
    Inv_AddItem(inv, 11, { "Heheh 11", IC_ACCESSORY });
    Inv_AddItem(inv, 12, { "Heheh 12", IC_ACCESSORY });

    Inv_DeleteItem(inv, 5);
    Inv_SearchItem(inv, "Heheh 7");
}

InvResult Inv_AddItem(Inventory& inv, item_id_t id, ItemMeta&& meta)
{
    if (InvUtil_FindItemById(inv, id) != nullptr)
        return InvResult::ErrDuplicateEntry;

    if (inv.count == inv.capacity)
    {
        auto old_list = inv.items;

        inv.capacity = grow(inv.capacity);
        inv.items = new InventoryItem[inv.capacity];

        if (inv.count > 0)
        {
            memcpy(inv.items, old_list, sizeof(InventoryItem) * inv.count);

            delete[] old_list;
        }
    }

    InventoryItem& slot = inv.items[inv.count++];

    slot.item_id = id;

    // slot.meta.name = meta.name;
    strcpy(slot.meta.name, meta.name);
    slot.meta.cat = meta.cat;

    slot.item_count = 0;
    slot.allocated_to = nullptr;

    return InvResult::Ok;
}

InvResult Inv_ViewItems(Inventory& inv)
{
    if (inv.count == 0)
        return InvResult::WarnEmpty;

    for (int i = 0; i < inv.count; ++i)
    {
        auto& item = inv.items[i];
        cout << "Inv_ViewItems(): " << item.meta.name << endl;
    }

    return InvResult::Ok;
}

InvResult Inv_SearchItem(Inventory& inv, const std::string& str)
{
    int count;
    for (int i = 0; i < inv.count; ++i)
    {
        auto& item = inv.items[i];
        if (item.active && item.meta.name == str)
        {
            ++count;
            /* output */
            cout << "Inv_SearchItem(): " << item.meta.name << endl;
        }
    }

    if (count == 0)
        return InvResult::WarnEmpty;

    return InvResult::Ok;
}

InvResult Inv_EditItem(Inventory& inv, item_id_t id, ItemMeta&& meta)
{
    auto item = InvUtil_FindItemById(inv, id);

    if (item == nullptr)
        return InvResult::ErrNotFound;

    // item->meta.name = std::move(meta.name);
    strcpy(item->meta.name, meta.name);
    item->meta.cat = meta.cat;

    return InvResult::Ok;
}


InvResult Inv_DeleteItem(Inventory& inv, item_id_t id)
{
    auto item = InvUtil_FindItemById(inv, id);

    if (item == nullptr)
        return InvResult::ErrNotFound;

    item->active = false;

    return InvResult::Ok;
}

Member* FindMemberByName(Member* head, const std::string& name)
{
    while (head != nullptr)
    {
        if (head->name == name)
            return head;

        head = head->next;
    }

    return nullptr;
}

InvResult Inv_AssignItem(Inventory& inv, item_id_t id, const std::string& to)
{
    auto item = InvUtil_FindItemById(inv, id);

    if (item == nullptr)
        return InvResult::ErrNotFound;

    auto entry = FindMemberByName(item->allocated_to, to);

    if (entry == nullptr)
    {
        entry = CreateMember(to);

        auto tail = item->allocated_to;

        if (tail != nullptr)
            tail->prev = entry;

        item->allocated_to = entry;

        entry->next = tail;
        entry->prev = nullptr;
    }

    ++entry->borrow_count;
    --item->item_count;

    return InvResult::Ok;
}

InvResult Inv_RetrieveItem(Inventory& inv, item_id_t id, const std::string& from)
{
    auto item = InvUtil_FindItemById(inv, id);

    if (item == nullptr)
        return InvResult::ErrNotFound;

    auto entry = FindMemberByName(item->allocated_to, from);

    if (entry == nullptr || entry->borrow_count == 0)
        return InvResult::ErrInvalidPayload;

    if (--entry->borrow_count == 0)
    {
        auto first = entry->prev;
        auto second = entry->next;

        Member* head = first == nullptr ? item->allocated_to : first->next; 

        head->next = second;

        if (second != nullptr)
            second->prev = first;

        DeleteMember(entry);
    }

    ++item->item_count;

    return InvResult::Ok;
}


InvResult Inv_ItemDetails(Inventory& inv, item_id_t id)
{
    auto item = InvUtil_FindItemById(inv, id);

    if (item == nullptr)
        return InvResult::ErrNotFound;

    cout << "Inv_ItemDetails(): " << item->item_id << " | " << item->meta.name << endl;

    return InvResult::Ok;
}