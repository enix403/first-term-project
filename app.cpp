#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <limits>
#include <iomanip>
#include <cctype>
// #include <type_traits>

#include "repr.h"
#include "serialization.h"

using namespace std;

namespace Lifecycle
{
    static void Welcome();
    static void OnBeforeQuit(Serialization::DataFile f, Inventory& inv);

    static void InitInventory(Inventory* inv);
    static void FreeInventory(Inventory* inv);
};

namespace Input
{
    int64_t             integer(bool allow_empty = false);
    bool                entity_name(char* target, size_t max_len, bool allow_empty = false);
    InventoryItem*      identitfied_item(Inventory& inv, bool show_error = true);
}; // namespace Input


namespace Frontend
{
    enum class MenuAction
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

    enum class ACResult
    {
        Ok = 0,
        Failed,
        Unrecoverable
    };

    MenuAction  RequestAction();
    ACResult    HandleAction(Inventory& inv, MenuAction action);

    ACResult AddItem(Inventory& inv);
    ACResult ViewItems(Inventory& inv);
    ACResult SearchItem(Inventory& inv);
    ACResult EditItem(Inventory& inv);
    ACResult DeleteItem(Inventory& inv);
    ACResult AssignItem(Inventory& inv);
    ACResult RetrieveItem(Inventory& inv);
    ACResult ItemDetails(Inventory& inv);
}; // namespace Frontend

namespace Core {

    static InventoryItem*   FindItemById(const Inventory& inv, item_id_t id, bool active_only = true);
    static Member*          FindMemberByName(Member* head, const char* name);

    static void Add(Inventory& inv, item_id_t id, item_count_t icount, const ItemMeta& meta);   
    static void Delete(Inventory& inv, InventoryItem* item);
    static void Assign(InventoryItem* item, const char* name);
    static void Assign(Inventory& inv, item_id_t id, const char* name);
    static void Retrieve(InventoryItem* item, Member* entry);  
}

int main()
{
    std::ios::sync_with_stdio(false);

    // Lifecycle::Welcome();

    Inventory inv;
    Lifecycle::InitInventory(&inv);

#if 0
    Core::Add(inv, 1, 41, { "Test Item 1", IC_MACHINERY });
    Core::Add(inv, 2, 41, { "Test Item 1", IC_MACHINERY });
    Core::Add(inv, 3, 41, { "Test Item 1", IC_MACHINERY });
    Core::Add(inv, 4, 41, { "Test Item 1", IC_MACHINERY });
    Core::Add(inv, 5, 41, { "Test Item 1", IC_MACHINERY });
    Core::Add(inv, 6, 41, { "Test Item 1", IC_MACHINERY });

    Core::Assign(inv, 3, "abc");
    Core::Assign(inv, 3, "def");

    Core::Assign(inv, 4, "abc");
    Core::Assign(inv, 4, "Hello");
    Core::Assign(inv, 4, "Hello 2");
    Core::Assign(inv, 4, "Hello");

    Core::Assign(inv, 5, "LMN");
    Core::Assign(inv, 6, "OPQ");
#endif

    auto file = Serialization::OpenFile();
    {
        using namespace Serialization;
        if (file == nullptr)
        {
            cout << "[ERROR] Unable to open file" << endl;
            return 1;
        }

        if (IsFileValid(file))
        {
            if (!ReadFromFile(file, inv))
            {
                cout << "[WARN] Invalid or corrupted file -- Skipping" << endl; \

                // Reset in case of failed read
                Lifecycle::FreeInventory(&inv);
                Lifecycle::InitInventory(&inv);
            }
        }
    }

    bool first_tick = true;

    for (;;)
    {
        using namespace Frontend;

        if (!first_tick)
        {
            cout << "----------------------------";
            cout << "\n\n";
        }

        first_tick = false;

        auto action = RequestAction();

        if (action == MenuAction::Quit)
        {
            break;
        }

        if (HandleAction(inv, action) == ACResult::Ok)
            Serialization::WriteToFile(file, inv);
    }

    Lifecycle::OnBeforeQuit(file, inv);
    Serialization::CloseFile(file);

    Lifecycle::FreeInventory(&inv);

    return 0;
}

namespace Lifecycle
{
    static inline void Welcome()
    {
        cout << "* Welcome to PUCIT Inventory Management System *\n" << endl;
    }

    static void OnBeforeQuit(Serialization::DataFile f, Inventory& inv)
    {
        Serialization::WriteToFile(f, inv);
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
};

namespace Input
{
    static constexpr streamsize max_ssz = std::numeric_limits<streamsize>::max();

    int64_t integer(bool allow_empty)
    {
        static const auto MAX_LINE_SIZE = 1024ll;
        static char line[MAX_LINE_SIZE];
        std::stringstream ss;

        std::cin.getline(line, MAX_LINE_SIZE, '\n');

        ss << line;

        // If string is empty then it's first char must be '\0'
        if (allow_empty && line[0] == '\0')
            return -2;

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

    bool entity_name(char* target, size_t max_len, bool allow_empty)
    {
        static const auto MAX_LINE_SIZE = 1024;
        static char name[MAX_LINE_SIZE];
        if (!allow_empty)
            cin >> ws;
        std::cin.getline(name, MAX_LINE_SIZE, '\n');

        auto xc = std::cin.gcount() - 1;

        if (std::cin.eof())
            return false;

        if (std::cin.fail() || xc >= max_len)
        {
            cout << "[ERROR] * Enter a name of less than " << max_len << "characters *\n";

            return false;
        }

        memcpy(target, name, xc + 1);

        return true;
    }

    InventoryItem* identitfied_item(Inventory& inv, bool show_error)
    {
        auto id_ = Input::integer();

        if (id_ == -1)
        {
            cout << "\n[ERROR] * Invalid id *" << '\n';
            return nullptr;
        }

        auto itemptr = Core::FindItemById(inv, (item_id_t) id_);

        if (show_error && itemptr == nullptr)
        {
            cout << "\n[ERROR] * Item with id " << id_ << " not found *\n"
                 << "        * Failed to load item *\n";
        }

        return itemptr;
    }

}; // namespace Input

namespace DisplayItem
{
    static constexpr int w1 =  6;
    static constexpr int w2 = 18;
    static constexpr int w3 = 18;
    static constexpr int w4 = 18;

    static inline void Header()
    {
        cout
            << std::setw(w1) << std::left << "ID"
            << std::setw(w2) << std::left << "Name"
            << std::setw(w3) << std::left << "Units Available"
            << std::setw(w4) << std::left << "Units Assigned"
        << "\n";

        cout
            << std::setw(w1 + w2 + w3 + w4)
            << std::setfill('-') << "" << "\n" << std::setfill(' ');
    }

    static inline void Summary(InventoryItem& item)
    {
        cout
            << std::setw(w1) << std::left << item.item_id
            << std::setw(w2) << std::left << item.meta.name
            << std::setw(w3) << std::left << item.item_count
            << std::setw(w4) << std::left << item.assigned_count
            << "\n";
    }

    static inline uint32_t MemList(InventoryItem& item)
    {
        auto mem = item.allocated_to;
        int i = 0;
        if (mem != nullptr)
        {
            cout << "\nAssigned To: \n";
            while (mem != nullptr)
            {
                cout
                    << "   > " << ++i << ". "
                    << mem->name << " | "
                    << mem->borrow_count << " unit(s) assigned" << "\n"; 
                mem = mem->next;
            }
        }

        return i;
    }

    uint32_t Full(InventoryItem& item)
    {
        Header();
        Summary(item);
        return MemList(item);
    }

    inline static void Compact(InventoryItem& item)
    {
        Summary(item);
    }
};

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

    MenuAction RequestAction()
    {
        cout << menu_ << endl;

        int op;

        while (true)
        {
            cout << "> Choose option [0-8]: ";

            bool valid = false;
            op = Input::integer();

            if (cin.eof())
                return MenuAction::Quit;

            if (op >= 0 && op <= 8)
                valid = true;

            if (valid)
                break;
            else
                cout << "[ERROR] * Invalid choice. Try again *" << endl;
        }

        return static_cast<MenuAction>(op);
    }

    ACResult HandleAction(Inventory& inv, MenuAction action)
    {
        cout << "\n";

        ACResult result = ACResult::Failed;

        switch (action)
        {
            case MenuAction::AddItem:      result = Frontend::AddItem(inv);      break;
            case MenuAction::ViewItems:    result = Frontend::ViewItems(inv);    break;
            case MenuAction::SearchItem:   result = Frontend::SearchItem(inv);   break;
            case MenuAction::EditItem:     result = Frontend::EditItem(inv);     break;
            case MenuAction::DeleteItem:   result = Frontend::DeleteItem(inv);   break;
            case MenuAction::AssignItem:   result = Frontend::AssignItem(inv);   break;
            case MenuAction::RetrieveItem: result = Frontend::RetrieveItem(inv); break;
            case MenuAction::ItemDetails:  result = Frontend::ItemDetails(inv);  break;

            default:
                break;
        }

        cout << endl;

        return result;
    }

} // namespace Frontend

namespace Frontend
{
    static const char* IDN = " >> ";

    ACResult AddItem(Inventory& inv)
    {
        item_id_t id;
        item_count_t icount;
        ItemMeta meta;

        {
            cout << IDN << "Enter Item Id: ";
            auto id_ = Input::integer();

            if (id_ == -1)
            {
                cout << "\n[ERROR] * Invalid id *" << '\n';
                return ACResult::Failed;
            }

            id = id_;

            if (Core::FindItemById(inv, id) != nullptr)
            {
                cout << "\n[ERROR] * Item with id " << id << " already exists. *\n"
                     << "        * Failed to add item *\n";

                return ACResult::Failed;
            }
        }

        {
            cout << IDN << "Enter Item name: ";
            if (!Input::entity_name(meta.name, MAX_NAME_LEN))
                return ACResult::Failed;
        }

        {
            cout << IDN << "Enter Item's available unit count: ";
            auto ic = Input::integer();

            if (ic == -1)
            {
                cout << "\n[ERROR] * Invalid input *" << '\n';
                return ACResult::Failed;
            }

            icount = ic;
        }

        meta.cat = IC_MACHINERY;

        cout << "\n";

        Core::Add(inv, id, icount, meta);

        cout << "Item \"" << meta.name << "\" added successfully\n";

        return ACResult::Ok;
    }

    ACResult ViewItems(Inventory& inv)
    {
        if (inv.count == 0)
        {
            cout << "*No items added*\n";
            return ACResult::Failed;
        }

        DisplayItem::Header();

        for (int i = 0; i < inv.count; ++i)
        {
            auto& item = inv.items[i];

            if (!item.active)
                continue;

            DisplayItem::Compact(item);
        }

        return ACResult::Ok;
    }

    ACResult SearchItem(Inventory& inv)
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
                DisplayItem::Full(inv.items[i]);
            }
        }

        if (count == 0)
        {
            cout << "*No items found*\n";
            return ACResult::Failed;
        }

        return ACResult::Ok;
    }

#define SELECT_ITEM(inv, item) \
    { \
        ACResult res; \
        if ((res = ViewItems(inv)) != ACResult::Ok) \
            return res; \
        cout << "\n"; \
    } \
    cout << IDN << "Enter Item Id: "; \
    auto item = Input::identitfied_item(inv); \
    if (item == nullptr) \
        return ACResult::Failed; \

    ACResult EditItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        cout << IDN << "Enter Item's new name (press enter to keep original): ";
        static name_str_t name;
        if (!Input::entity_name(name, MAX_NAME_LEN, true))
            return ACResult::Failed;

        cout << IDN << "Enter Item's available unit count (press enter to keep original): ";
        auto ic = Input::integer(true);
        if (ic == -1)
        {
            cout << "\n[ERROR] * Invalid input *" << '\n';
            return ACResult::Failed;
        }

        cout << "\n";

        item->meta.cat = IC_MACHINERY; // Edited

        if (ic != -2)
            item->item_count = ic;

        if (name[0] != '\0') /* Name is not empty, i.e user has provided some input */
            strcpy(item->meta.name, name);

        cout << "Item saved successfully\n";

        return ACResult::Ok;
    }

    ACResult DeleteItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        if (item == nullptr)
            return ACResult::Failed;

        cout << "\n";

        Core::Delete(inv, item);

        cout << "Item \"" << item->meta.name << "\" with id " << item->item_id << " deleted successfully\n";

        return ACResult::Ok;
    }

    ACResult AssignItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        if (item->item_count > 0)
        {
            cout << "\n<*> Assigning item \"" << item->meta.name << "\"\n";
        }
        else
        {
            cout << "\n[ERROR] * No units available for this item *" << '\n';
            return ACResult::Failed;
        }

        static name_str_t name;
        cout << IDN << "Enter assignee's name: ";
        if (!Input::entity_name(name, MAX_NAME_LEN))
            return ACResult::Failed;

        cout << "\n";

        Core::Assign(item, name);

        cout
            << "Item \"" << item->meta.name
            << "\" assigned to \"" << name << "\" successfully\n";

        return ACResult::Ok;
    }

    ACResult RetrieveItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        if (item->assigned_count == 0)
        {
            cout << "\n* No units currently assigned to any member *" << '\n';
            return ACResult::Ok;
        }

        auto mem_count = DisplayItem::Full(*item);

        cout << IDN << "Select an entry: ";
        auto location = Input::integer();

        if (location < 1 || location > mem_count)
        {
            cout << "\n [ERROR] * Invalid choice *" << '\n';
            return ACResult::Failed;
        }
        --location;

        cout << "\n";

        auto entry = item->allocated_to;
        for (int i = 0; i < location; ++i)
            entry = entry->next;

        /* TODO: check for unreachable case of entry->borrow_count == 0 */
        Core::Retrieve(item, entry);

        cout
            << "Item \"" << item->meta.name << "\" retrieved successfully\n";

        return ACResult::Ok;
    }

    ACResult ItemDetails(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        cout << "\n";

        DisplayItem::Full(*item);

        return ACResult::Ok;
    }
}; // namespace Core

namespace Core {

    InventoryItem* FindItemById(const Inventory& inv, item_id_t id, bool active_only)
    {
        for (int i = 0; i < inv.count; ++i)
        {
            auto& item = inv.items[i];
            if (item.item_id == id && (!active_only || item.active))
                return &item;
        }

        return nullptr;
    }

    Member* FindMemberByName(Member* head, const char* name)
    {
        while (head != nullptr)
        {
            if (strcmp(head->name, name) == 0)
                return head;

            head = head->next;
        }

        return nullptr;
    }

    static void Add(Inventory& inv, item_id_t id, item_count_t icount, const ItemMeta& meta)
    {
        if (inv.count == inv.capacity)
            InvUtil_AllocateFor(inv, grow(inv.capacity));

        InventoryItem& slot = inv.items[inv.count++];

        slot.item_id = id;

        strcpy(slot.meta.name, meta.name);
        slot.meta.cat = meta.cat;

        slot.item_count = icount;
        slot.allocated_to = nullptr;
    }
 
    static inline void Delete(Inventory& inv, InventoryItem* item)
    {
        item->active = false;
    }

    static void Assign(InventoryItem* item, const char* name)
    {
        auto entry = FindMemberByName(item->allocated_to, name);

        if (entry == nullptr)
        {
            entry = CreateMember(name);

            auto tail = item->allocated_to;

            if (tail != nullptr)
                tail->prev = entry;

            item->allocated_to = entry;

            entry->next = tail;
            entry->prev = nullptr;
        }

        ++entry->borrow_count;
        ++item->assigned_count;
        --item->item_count;
    }

    static inline void Assign(Inventory& inv, item_id_t id, const char* name)
    {
        auto item = FindItemById(inv, id);
        if (item != nullptr)
            Assign(item, name);
    }

    static void Retrieve(InventoryItem* item, Member* entry)
    {
        if (--entry->borrow_count == 0)
        {
            auto first = entry->prev;
            auto second = entry->next;

            Member** head = first == nullptr ?
                &item->allocated_to : &first->next;

            *head = second;

            if (second != nullptr)
                second->prev = first;

            DeleteMember(entry);
        }

        ++item->item_count;
        --item->assigned_count;
    }
}

