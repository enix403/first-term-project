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

enum class K__Result
{
    Ok = 0,
    Failed,
    Unrecoverable
};

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


// auto __trival = std::is_trivially_copyable<InventoryItem>::value;
// auto __sz = sizeof(InventoryItem);

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

InventoryItem* InvUtil_FindItemById(const Inventory& inv, item_id_t id, bool active_only = true);
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
    InventoryItem* identitfied_item(const Inventory& inv, bool show_error = true);
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

    // InvResult AddItem(Inventory& inv, item_id_t id, const ItemMeta& meta);
    K__Result AddItem(Inventory& inv);
    K__Result ViewItems(Inventory& inv);
    K__Result SearchItem(Inventory& inv);
    K__Result EditItem(Inventory& inv);
    K__Result DeleteItem(Inventory& inv);
    // InvResult AssignItem(Inventory& inv, item_id_t id, const std::string& to);
    // InvResult RetrieveItem(Inventory& inv, item_id_t id, const std::string& from);
    K__Result ItemDetails(Inventory& inv);

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

#if 1
    // clang-format off
    inv.items[inv.count++] = { 1, { "Item 1", IC_MACHINERY } };
    inv.items[inv.count++] = { 2, { "Item 2", IC_MACHINERY } };
    inv.items[inv.count++] = { 3, { "Item 3", IC_MACHINERY } };
    inv.items[inv.count++] = { 4, { "Item 4", IC_MACHINERY } };
    inv.items[inv.count++] = { 5, { "Item 5", IC_MACHINERY } };
    inv.items[inv.count++] = { 6, { "Item 6", IC_MACHINERY } };
    // clang-format on
#endif

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
            cout << "[ERROR] * Enter a name of less than " << MAX_NAME_LEN << "characters *\n";

            return false;
        }

        memcpy(target, name, xc + 1);

        return true;
    }

    InventoryItem* identitfied_item(const Inventory& inv, bool show_error)
    {
        auto id_ = Input::integer();

        if (id_ == -1)
        {
            cout << "\n[ERROR] * Invalid id *" << '\n';
            return nullptr;
        }

        auto itemptr = InvUtil_FindItemById(inv, (item_id_t)id_);

        if (show_error && itemptr == nullptr)
        {
            cout << "\n[ERROR] * Item with id " << id_ << " not found *\n"
                 <<   "        * Failed to load item *\n";
        }

        return itemptr;
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
                cout << "[ERROR] * Invalid choice. Try again *" << endl;
        }

        return static_cast<Action>(op);
    }

    void HandleAction(Inventory& inv, Action action)
    {
        cout << "\n";

        switch (action)
        {
            case Action::AddItem:       InvUserActions::AddItem(inv);       break;
            case Action::ViewItems:     InvUserActions::ViewItems(inv);     break;
            case Action::SearchItem:    InvUserActions::SearchItem(inv);    break;
            case Action::EditItem:      InvUserActions::EditItem(inv);      break;
            case Action::DeleteItem:    InvUserActions::DeleteItem(inv);    break;
            case Action::ItemDetails:   InvUserActions::ItemDetails(inv);   break;

            default:
                break;
        }

        cout << endl;
    }

} // namespace Frontend

namespace InvUserActions
{
    static const char* IDN = " >> ";

    K__Result AddItem(Inventory& inv)
    {
        item_id_t id;
        ItemMeta meta;

        {
            cout << IDN << "Enter Item Id: ";
            auto id_ = Input::integer();

            if (id_ == -1)
            {
                cout << "\n[ERROR] * Invalid id *" << '\n';
                return K__Result::Failed;
            }

            id = id_;

            if (InvUtil_FindItemById(inv, id) != nullptr)
            {
                cout << "\n[ERROR] * Item with id " << id << " already exists. *\n"
                     <<   "        * Failed to add item *\n";

                return K__Result::Failed;
            }

        }

        cout << IDN << "Enter Item name: ";
        if (!Input::entity_name(meta.name, MAX_NAME_LEN))
            return K__Result::Failed;

        meta.cat = IC_MACHINERY;

        cout << "\n";

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

        return K__Result::Ok;
    }

    K__Result ViewItems(Inventory& inv)
    {
        if (inv.count == 0)
        {
            cout << "*No items added*\n";
            return K__Result::Ok;
        }

        for (int i = 0; i < inv.count; ++i)
            Frontend::DisplayItem(inv.items[i]);

        return K__Result::Ok;
    }

    K__Result SearchItem(Inventory& inv)
    {
        cout << IDN << "Enter Item Name: "; 
        string str;
        getline(std::cin >> std::ws, str);

        cout << "\n";

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

        return K__Result::Ok;
    }

    K__Result EditItem(Inventory& inv)
    {
        cout << IDN << "Enter Item Id: ";
        auto item = Input::identitfied_item(inv);

        if (item == nullptr)
            return K__Result::Failed;

        cout << IDN << "Enter Item's new name: ";
        if (!Input::entity_name(item->meta.name, MAX_NAME_LEN))
            return K__Result::Failed;

        cout << "\n";

        item->meta.cat = IC_MACHINERY; // Edited

        cout << "Item saved successfully\n";

        return K__Result::Ok;
    }

    K__Result DeleteItem(Inventory& inv)
    {
        cout << IDN << "Enter Item Id: ";
        auto item = Input::identitfied_item(inv);

        if (item == nullptr)
            return K__Result::Failed;

        cout << "\n";

        item->active = false;

        cout << "Item \"" << item->meta.name
            << "\" with id " << item->item_id
            << " deleted successfully\n";

        return K__Result::Ok;
    }

#if 0

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

#endif

    K__Result ItemDetails(Inventory& inv)
    {
        cout << IDN << "Enter Item Id: ";
        auto item = Input::identitfied_item(inv);

        if (item == nullptr)
            return K__Result::Failed;

        cout << "\n";

        Frontend::DisplayItem(*item);

        return K__Result::Ok;
    }
}; // namespace InvUserActions

InventoryItem* InvUtil_FindItemById(const Inventory& inv, item_id_t id, bool active_only)
{
    for (int i = 0; i < inv.count; ++i)
    {
        auto& item = inv.items[i];
        if (item.item_id == id && (!active_only || item.active))
            return &item;
    }

    return nullptr;
}
