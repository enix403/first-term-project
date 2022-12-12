#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <limits>
#include <iomanip>
#include <cstdint>
#include <type_traits>

using namespace std;

enum ItemCategory
{
    IC_STATIONARY,
    IC_MACHINERY,
    IC_ACCESSORY,
};

struct Member
{
    char name[50];
    int borrow_count = 0;

    Member* prev = nullptr;
    Member* next = nullptr;
};

typedef uint16_t item_id_t;

struct ItemMeta
{
    char name[50];
    ItemCategory cat;
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
};

inline constexpr uint32_t grow(uint32_t old)
{
    return old < 8 ? 8 : 2 * old;
}

inline Member* CreateMember(const std::string& name)
{
    Member* m = new Member;

    strcpy(m->name, name.c_str());
    m->borrow_count = 0;
    m->next = nullptr;
    m->prev = nullptr;

    return m;
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

namespace InvUserActions
{
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

    InvResult AddItem(Inventory& inv, item_id_t id, ItemMeta&& meta);
    InvResult ViewItems(Inventory& inv);
    InvResult SearchItem(Inventory& inv, const std::string& str);
    InvResult EditItem(Inventory& inv, item_id_t id, ItemMeta&& item);
    InvResult DeleteItem(Inventory& inv, item_id_t id);
    InvResult AssignItem(Inventory& inv, item_id_t id, const std::string& to);
    InvResult RetrieveItem(Inventory& inv, item_id_t id, const std::string& from);
    InvResult ItemDetails(Inventory& inv, item_id_t id);

}; // namespace InvUserActions

namespace Frontend
{
    enum class Action
    {
        Quit = 0,

        AddItem,
        ViewItems,
        SearchItem,
        EditItem,
        DeleteItem,
        AssignItem,
        RetrieveItem,
        ItemDetails,
    };

    int IntegerInput();
    Action RequestAction();
}; // namespace Frontend

int main()
{
    std::ios::sync_with_stdio(false);

    Inventory inv;

    inv.count = 0;
    inv.capacity = 1 << 7;
    inv.items = new InventoryItem[inv.capacity];

    // auto val = Frontend::IntegerInput();

    // cout << "Val=" << val << endl;

    auto action = Frontend::RequestAction();

    // InvUserActions::AddItem(inv, 1, { "Heheh 1", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 2, { "Heheh 2", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 3, { "Heheh 3", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 4, { "Heheh 4", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 5, { "Heheh 5", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 6, { "Heheh 6", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 7, { "Heheh 7", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 8, { "Heheh 8", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 9, { "Heheh 9", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 10, { "Heheh 10", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 11, { "Heheh 11", IC_ACCESSORY });
    // InvUserActions::AddItem(inv, 12, { "Heheh 12", IC_ACCESSORY });

    // InvUserActions::DeleteItem(inv, 5);
    // InvUserActions::SearchItem(inv, "Heheh 7");
}

namespace Frontend
{
    static const char* menu_ = 1 + (const char*)R"(
Select an option:
    [0] Quit 
    [1] Add a New tem
    [2] View Added Items
    [3] Search Item By Name
    [4] Edit an Existing Item
    [5] Delete an Existing Item
    [6] Assign an Existing Item to a Member
    [7] Retrieve an Existing Item from a Member
    [8] Show Details of a Specifc Item
)";

    constexpr streamsize max_ssz = std::numeric_limits<streamsize>::max(); 

    int IntegerInput()
    {
        static const auto MAX_LINE_SIZE = 1024;
        static char line[MAX_LINE_SIZE];
        std::stringstream ss;
        
        std::cin.getline(line, MAX_LINE_SIZE, '\n');

        ss << line;

        int val;
        ss >> std::skipws >> val;

        if (ss.fail())
            return -1;

        while (true)
        {
            char c = ss.get();
            if (ss.eof())
                break;

            if (c != ' ')
                return -1;
        }

        return val;
    }

    Action RequestAction()
    {
        cout << menu_ << endl;

        int op;

        while (true)
        {
            cout << ">> Choose option [0-8]: ";

            bool valid = false;
            op = IntegerInput();

            if (cin.eof())
                return Action::Quit;

            if (op >= 0 && op <= 8)
                valid = true;

            if (valid)
                break;
            else
                cout << "*Invalid choice* try again" << endl;
        }

        return static_cast<Action>(op);
    }
} // namespace Frontend

namespace InvUserActions
{
    InvResult AddItem(Inventory& inv, item_id_t id, ItemMeta&& meta)
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

    InvResult ViewItems(Inventory& inv)
    {
        if (inv.count == 0)
            return InvResult::WarnEmpty;

        for (int i = 0; i < inv.count; ++i)
        {
            auto& item = inv.items[i];
            cout << "ViewItems(): " << item.meta.name << endl;
        }

        return InvResult::Ok;
    }

    InvResult SearchItem(Inventory& inv, const std::string& str)
    {
        int count;
        for (int i = 0; i < inv.count; ++i)
        {
            auto& item = inv.items[i];
            if (item.active && item.meta.name == str)
            {
                ++count;
                /* output */
                cout << "SearchItem(): " << item.meta.name << endl;
            }
        }

        if (count == 0)
            return InvResult::WarnEmpty;

        return InvResult::Ok;
    }

    InvResult EditItem(Inventory& inv, item_id_t id, ItemMeta&& meta)
    {
        auto item = InvUtil_FindItemById(inv, id);

        if (item == nullptr)
            return InvResult::ErrNotFound;

        // item->meta.name = std::move(meta.name);
        strcpy(item->meta.name, meta.name);
        item->meta.cat = meta.cat;

        return InvResult::Ok;
    }


    InvResult DeleteItem(Inventory& inv, item_id_t id)
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

    InvResult AssignItem(Inventory& inv, item_id_t id, const std::string& to)
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

    InvResult RetrieveItem(Inventory& inv, item_id_t id, const std::string& from)
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


    InvResult ItemDetails(Inventory& inv, item_id_t id)
    {
        auto item = InvUtil_FindItemById(inv, id);

        if (item == nullptr)
            return InvResult::ErrNotFound;

        cout << "ItemDetails(): " << item->item_id << " | " << item->meta.name << endl;

        return InvResult::Ok;
    }
}; // namespace InvUserActions