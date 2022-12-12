#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <limits>
#include <iomanip>
#include <cstdint>
#include <cctype>
#include <type_traits>

using namespace std;

enum ItemCategory
{
    IC_STATIONARY,
    IC_MACHINERY,
    IC_ACCESSORY,
};

static constexpr size_t MAX_NAME_LEN = 50;

struct Member
{
    char name[MAX_NAME_LEN];
    int borrow_count = 0;

    Member* prev = nullptr;
    Member* next = nullptr;
};

typedef uint16_t item_id_t;

struct ItemMeta
{
    char name[MAX_NAME_LEN];
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

InventoryItem* InvUtil_FindItemById(Inventory& inv, item_id_t id, bool active_only = true);
void InitInventory(Inventory* inv);
void FreeInventory(Inventory* inv);

inline void Welcome()
{
    cout << "* Welcome to PUCIT Inventory Management System *\n" << endl;
}
void LoadFromFile(Inventory& inv);
void OnBeforeQuit(Inventory& inv);

namespace Input
{
    int64_t integer();
    void entity_name();
}; // namespace Input

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

    // InvResult AddItem(Inventory& inv, item_id_t id, ItemMeta&& meta);
    InvResult AddItem(Inventory& inv, item_id_t id, const ItemMeta& meta);
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

    inline void DisplayItem(InventoryItem& item)
    {
        cout << item.item_id << " | " << item.meta.name << endl;   
    }

    Action RequestAction();
    void HandleAction(Inventory& inv, Action action);

}; // namespace Frontend

int main()
{
    std::ios::sync_with_stdio(false);

    Welcome();

    Inventory inv;
    InitInventory(&inv);

    bool first_tick = true;

    for (;;)
    {
        if (!first_tick)
        {
            cout << "----------------------------";
            cout << "\n\n";
        }

        first_tick = false;

        auto action = Frontend::RequestAction();

        if (action == Frontend::Action::Quit)
        {
            break;
        }

        Frontend::HandleAction(inv, action);
    }

    OnBeforeQuit(inv);
    FreeInventory(&inv);

    return 0;
}

void InitInventory(Inventory* inv)
{
    inv->count = 0;
    inv->capacity = 256;
    inv->items = new InventoryItem[inv->capacity];
}

void FreeInventory(Inventory* inv)
{
    /* TODO: free nested member lists */
    delete inv->items;
}

void LoadFromFile(Inventory& inv) {}
void OnBeforeQuit(Inventory& inv) {}

namespace Input
{
    constexpr streamsize max_ssz = std::numeric_limits<streamsize>::max();

    int64_t integer()
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

            if (!std::isspace(c))
                return -1;
        }

        return val < 0 ? -1 : val;
    }

    bool entity_name(char* target, size_t max_len)
    {
        static const auto MAX_LINE_SIZE = 1024;
        static char name[MAX_LINE_SIZE];
        cin >> ws;
        std::cin.getline(name, MAX_LINE_SIZE, '\n');

        auto xc = std::cin.gcount() - 1;

        if (std::cin.eof())
            return false;

        if (std::cin.fail() || xc >= max_len)
        {
            cout << "[ERROR] Enter a name of less than " << MAX_NAME_LEN << "characters\n";

            return false;
        }

        memcpy(target, name, xc + 1);

        return true;
    }
}; // namespace Input

namespace Frontend
{
    static const char* menu_ = 1 + (const char*) R"(
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


    Action RequestAction()
    {
        cout << menu_ << endl;

        int op;

        while (true)
        {
            cout << "> Choose option [0-8]: ";

            bool valid = false;
            op = Input::integer();

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

    void HandleAction(Inventory& inv, Action action)
    {
        cout << "\n";

        static const char* IDN = " >> ";

        switch (action)
        {
            case Action::AddItem: {
                item_id_t id;
                ItemMeta meta;

                cout << IDN << "Enter Item Id: ";
                auto id_ = Input::integer();

                if (id_ == -1)
                {
                    cout << "[ERROR] Invalid id" << '\n';
                    break;
                }

                id = id_;

                cout << IDN << "Enter Item name: ";
                if (!Input::entity_name(meta.name, MAX_NAME_LEN))
                    break;

                meta.cat = IC_MACHINERY;

                InvUserActions::AddItem(inv, id, meta);

                break;
            }

            case Action::ViewItems:
                InvUserActions::ViewItems(inv);
                break;

            case Action::SearchItem: {
                cout << IDN << "Enter Item Name: "; 
                string line;
                getline(std::cin >> std::ws, line);

                InvUserActions::SearchItem(inv, line);

                break;
            }

            default:
                break;
        }

        cout << endl;
    }

} // namespace Frontend

namespace InvUserActions
{
    InvResult AddItem(Inventory& inv, item_id_t id, const ItemMeta& meta)
    {
        if (InvUtil_FindItemById(inv, id) != nullptr)
        {
            cout << "[ERROR] Item with id " << id << " already exists\n"
                 << "        Failed to add item\n";

            return InvResult::ErrDuplicateEntry;
        }

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

        cout << "\nItem \"" << meta.name << "\" added successfully\n";

        return InvResult::Ok;
    }

    InvResult ViewItems(Inventory& inv)
    {
        if (inv.count == 0)
        {
            cout << "*No items added*\n";
            return InvResult::Ok;
        }

        for (int i = 0; i < inv.count; ++i)
            Frontend::DisplayItem(inv.items[i]);

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
                Frontend::DisplayItem(inv.items[i]);
            }
        }

        if (count == 0)
        {
            cout << "*No items found*\n";
        }

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

InventoryItem* InvUtil_FindItemById(Inventory& inv, item_id_t id, bool active_only)
{
    for (int i = 0; i < inv.count; ++i)
    {
        auto& item = inv.items[i];
        if (item.item_id == id && (!active_only || item.active))
            return &item;
    }

    return nullptr;
}
