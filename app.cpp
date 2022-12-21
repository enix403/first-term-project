#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <iomanip>

#include "repr.h"
#include "serialization.h"

using namespace std;

namespace Lifecycle
{
    static void Welcome();
    static void OnBeforeQuit(Serialization::DataFile f, Inventory& inv);

    static void InitInventory(Inventory* inv);
    static void FreeInventory(Inventory* inv);
}; // namespace Lifecycle

namespace Input
{
    bool string(std::string& target, bool allow_empty = false);
    int64_t integer(bool allow_empty = false);
    InventoryItem* identitfied_item(Inventory& inv, bool show_error = true);
}; // namespace Input

namespace Core
{
    static InventoryItem* FindItemById(const Inventory& inv, item_id_t id, bool active_only = true);
    static Member* FindMemberByName(Member* head, const char* name);

    static void Add(Inventory& inv, item_id_t id, item_count_t icount, const ItemMeta& meta);
    static void Delete(InventoryItem* item);
    static void Assign(InventoryItem* item, const char* name);
    static void Assign(Inventory& inv, item_id_t id, const char* name);
    static void Retrieve(InventoryItem* item, Member* entry);
} // namespace Core

namespace Frontend
{
    enum class NextTickStatus
    {
        Quit,
        Continue
    };

    enum class InvActionResult
    {
        Ok = 0,
        Failed,
        Unrecoverable
    };

    struct TickInfo
    {
        InvActionResult result;
        NextTickStatus next_tick_st;
    };

    TickInfo AppTick(Inventory& inv);

    InvActionResult AddItem(Inventory& inv);
    InvActionResult ViewItems(Inventory& inv);
    InvActionResult SearchItem(Inventory& inv);
    InvActionResult EditItem(Inventory& inv);
    InvActionResult DeleteItem(Inventory& inv);
    InvActionResult AssignItem(Inventory& inv);
    InvActionResult RetrieveItem(Inventory& inv);
    InvActionResult ItemDetails(Inventory& inv);
}; // namespace Frontend

int main()
{
    std::ios::sync_with_stdio(false);

    Lifecycle::Welcome();

    Inventory inv;
    Lifecycle::InitInventory(&inv);

#if 0
    Core::Add(inv, 1, 41, { "Test Item 1", "Category 1" });
    Core::Add(inv, 2, 41, { "Test Item 2", "Category 2" });
    Core::Add(inv, 3, 41, { "Test Item 3", "Category 3" });
    Core::Add(inv, 4, 41, { "Test Item 4", "Category 4" });
    Core::Add(inv, 5, 41, { "Test Item 5", "Category 5" });
    Core::Add(inv, 6, 41, { "Test Item 6", "Category 6" });

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
            std::cerr << "[ERROR] Unable to open file" << endl;
            return 1;
        }

        if (IsFileValid(file))
        {
            if (!ReadFromFile(file, inv))
            {
                std::cerr << "[WARN] Invalid or corrupted file -- Skipping" << endl;

                // Reset in case of failed read
                Lifecycle::FreeInventory(&inv);
                Lifecycle::InitInventory(&inv);
            }
        }
    }

    bool first_tick = true;

    while (!std::cin.eof())
    {
        using namespace Frontend;

        if (!first_tick)
        {
            std::cout << "----------------------------";
            std::cout << "\n\n";
        }

        first_tick = false;

        auto tick = Frontend::AppTick(inv);

        if (tick.next_tick_st == Frontend::NextTickStatus::Quit)
            break;

        if (tick.result == InvActionResult::Ok)
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
        std::cout << "* Welcome to PUCIT Inventory Management System *\n" << endl;
    }

    static void OnBeforeQuit(Serialization::DataFile f, Inventory& inv)
    {
        Serialization::WriteToFile(f, inv);
    }

    void InitInventory(Inventory* inv)
    {
        inv->count = 0;
        inv->capacity = 0;
        inventory_allocate_capacity(*inv, 64);
    }

    void FreeInventory(Inventory* inv)
    {
        for (int i = 0; i < inv->count; ++i)
        {
            Member* head = inv->items[i].allocated_to;
            while (head != nullptr)
            {
                auto next = head->next;
                DeleteMember(head);
                head = next;
            }
        }

        delete[] inv->items;
    }
}; // namespace Lifecycle

namespace Input
{
    static constexpr streamsize max_ssz = std::numeric_limits<streamsize>::max();

    bool string(std::string& target, bool allow_empty)
    {
        if (!allow_empty)
            std::cin >> std::ws;

        static std::string buf;

        getline(std::cin, buf);

        if (std::cin.eof() || std::cin.fail())
        {
            return false;
        }

        target = buf;

        return true;
    }

    int64_t integer(bool allow_empty)
    {
        static std::string target;
        if (!Input::string(target, allow_empty))
            return -1;

        stringstream ss(target);

        /* Immediately discard leading whitespaces */
        ss >> std::ws;

        if (allow_empty && ss.eof())
            return -2;

        int64_t val;

        ss >> val;

        if (ss.fail())
            return -1;

        if (!(ss >> ws).eof())
            return -1;

        return val;
    }

    InventoryItem* identitfied_item(Inventory& inv, bool show_error)
    {
        auto id_ = Input::integer();

        if (id_ == -1)
        {
            std::cerr << "\n[ERROR] * Invalid id *" << '\n';
            return nullptr;
        }

        auto itemptr = Core::FindItemById(inv, (item_id_t) id_);

        if (show_error && itemptr == nullptr)
        {
            std::cerr << "\n[ERROR] * Item with id " << id_ << " not found *\n"
                      << "        * Failed to load item *\n";
        }

        return itemptr;
    }

}; // namespace Input

namespace DisplayItem
{
    static constexpr int w1 = 6;
    static constexpr int w2 = 16;
    static constexpr int w3 = 18;
    static constexpr int w4 = 18;
    static constexpr int w5 = 18;

    static inline void Header()
    {
        // clang-format off
        std::cout
            << std::setw(w1) << std::left << "ID"
            << std::setw(w2) << std::left << "Name"
            << std::setw(w3) << std::left << "Category"
            << std::setw(w4) << std::left << "Units Available"
            << std::setw(w5) << std::left << "Units Assigned"
        << "\n";

        std::cout
            << std::setw(w1 + w2 + w3 + w4 + w5)
            << std::setfill('-') << "" << "\n" << std::setfill(' ');
        // clang-format on
    }

    static inline void Summary(InventoryItem& item)
    {
        // clang-format off
        std::cout
            << std::setw(w1) << std::left << item.item_id
            << std::setw(w2) << std::left << item.meta.name
            << std::setw(w3) << std::left << item.meta.cat
            << std::setw(w4) << std::left << item.item_count
            << std::setw(w5) << std::left << item.assigned_count
            << "\n";
        // clang-format on
    }

    static inline uint32_t MemList(InventoryItem& item)
    {
        auto mem = item.allocated_to;
        int i = 0;
        if (mem != nullptr)
        {
            std::cout << "\nAssigned To: \n";
            while (mem != nullptr)
            {
                // clang-format off
                std::cout
                    << "   > " << ++i << ". "
                    << mem->name << " | "
                    << mem->borrow_count << " unit(s) assigned" << "\n";
                // clang-format on
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
}; // namespace DisplayItem

namespace Frontend
{
    static const char* g_menu = 1 + (const char*) R"(
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

    using menu_option_t = int64_t;

    static menu_option_t menu_input()
    {
        std::cout << g_menu << endl;

        menu_option_t op;

        while (true)
        {
            std::cout << "> Choose option [0-8]: ";

            bool valid = false;
            op = Input::integer();

            if (std::cin.eof())
                return 0;

            if (op >= 0 && op <= 8)
                valid = true;

            if (valid)
                break;
            else
                std::cerr << "[ERROR] * Invalid choice. Try again *" << endl;
        }

        return op;
    }

    TickInfo AppTick(Inventory& inv)
    {
        auto op = menu_input();

        InvActionResult result = InvActionResult::Failed;

        if (op == 0)
        {
            return { result, NextTickStatus::Quit };
        }

        // clang-format off
        switch (op)
        {
            case 1:     result = Frontend::AddItem(inv);      break;
            case 2:     result = Frontend::ViewItems(inv);    break;
            case 3:     result = Frontend::SearchItem(inv);   break;
            case 4:     result = Frontend::EditItem(inv);     break;
            case 5:     result = Frontend::DeleteItem(inv);   break;
            case 6:     result = Frontend::AssignItem(inv);   break;
            case 7:     result = Frontend::RetrieveItem(inv); break;
            case 8:     result = Frontend::ItemDetails(inv);  break;

            default:
                break;
        }
        // clang-format on

        std::cout << endl;

        return { result, NextTickStatus::Continue };
    }

} // namespace Frontend

namespace Frontend
{
    static const char* IDN = " >> ";

    InvActionResult AddItem(Inventory& inv)
    {
        item_id_t id;
        item_count_t icount;
        ItemMeta meta;

        {
            std::cout << IDN << "Enter Item Id: ";
            auto id_ = Input::integer();

            if (id_ == -1)
            {
                std::cout << "\n[ERROR] * Invalid id *" << '\n';
                return InvActionResult::Failed;
            }

            id = id_;

            if (Core::FindItemById(inv, id) != nullptr)
            {
                std::cerr << "\n[ERROR] * Item with id " << id << " already exists. *\n"
                          << "        * Failed to add item *\n";

                return InvActionResult::Failed;
            }
        }

        {
            std::cout << IDN << "Enter Item name: ";
            if (!Input::string(meta.name))
                return InvActionResult::Failed;
        }

        {
            std::cout << IDN << "Enter Item Category: ";
            if (!Input::string(meta.cat))
                return InvActionResult::Failed;
        }

        {
            std::cout << IDN << "Enter Item's available unit count: ";
            auto ic = Input::integer();

            if (ic == -1)
            {
                std::cerr << "\n[ERROR] * Invalid input *" << '\n';
                return InvActionResult::Failed;
            }

            icount = ic;
        }

        std::cout << "\n";

        Core::Add(inv, id, icount, meta);

        std::cout << "Item \"" << meta.name << "\" added successfully\n";

        return InvActionResult::Ok;
    }

    InvActionResult ViewItems(Inventory& inv)
    {
        if (inv.count == 0)
        {
            std::cout << "*No items added*\n";
            return InvActionResult::Failed;
        }

        DisplayItem::Header();

        for (int i = 0; i < inv.count; ++i)
        {
            auto& item = inv.items[i];

            if (!item.active)
                continue;

            DisplayItem::Compact(item);
        }

        return InvActionResult::Ok;
    }

    InvActionResult SearchItem(Inventory& inv)
    {
        string str;
        {
            std::cout << IDN << "Enter Item name: ";
            if (!Input::string(str))
                return InvActionResult::Failed;
        }

        std::cout << "\n";

        int count = 0;
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
            std::cout << "*No items found*\n";
            return InvActionResult::Failed;
        }

        return InvActionResult::Ok;
    }

// clang-format off
#define SELECT_ITEM(inv, item) \
    { \
        InvActionResult res; \
        if ((res = ViewItems(inv)) != InvActionResult::Ok) \
            return res; \
        std::cout << "\n"; \
    } \
    std::cout << IDN << "Enter Item Id: "; \
    auto item = Input::identitfied_item(inv); \
    if (item == nullptr) \
        return InvActionResult::Failed;
    // clang-format on

    InvActionResult EditItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        std::cout << IDN << "Enter Item's new name (press enter to keep original): ";
        static std::string name;
        if (!Input::string(name, true))
            return InvActionResult::Failed;

        std::cout << IDN << "Enter Item's new category (press enter to keep original): ";
        static std::string cat;
        if (!Input::string(cat, true))
            return InvActionResult::Failed;

        std::cout << IDN << "Enter Item's available unit count (press enter to keep original): ";
        auto ic = Input::integer(true);
        if (ic == -1)
        {
            std::cerr << "\n[ERROR] * Invalid input *" << '\n';
            return InvActionResult::Failed;
        }

        std::cout << "\n";

        if (ic != -2)
            item->item_count = ic;

        if (!(stringstream(name) >> std::ws).eof())
            item->meta.name = name;

        if (!(stringstream(cat) >> std::ws).eof())
            item->meta.cat = cat;

        std::cout << "Item saved successfully\n";

        return InvActionResult::Ok;
    }

    InvActionResult DeleteItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        std::cout << "\n";

        Core::Delete(item);

        std::cout << "Item \"" << item->meta.name << "\" with id " << item->item_id << " deleted successfully\n";
        return InvActionResult::Ok;
    }

    InvActionResult AssignItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        if (item->item_count > 0)
        {
            std::cout << "\n<*> Assigning item \"" << item->meta.name << "\"\n";
        }
        else
        {
            std::cerr << "\n[ERROR] * No units available for this item *" << '\n';
            return InvActionResult::Failed;
        }

        static std::string name;
        std::cout << IDN << "Enter assignee's name: ";
        if (!Input::string(name))
            return InvActionResult::Failed;

        std::cout << "\n";

        Core::Assign(item, name.c_str());

        std::cout << "Item \"" << item->meta.name << "\" assigned to \"" << name << "\" successfully\n";

        return InvActionResult::Ok;
    }

    InvActionResult RetrieveItem(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        if (item->assigned_count == 0)
        {
            std::cout << "\n* No units currently assigned to any member *" << '\n';
            return InvActionResult::Ok;
        }

        auto mem_count = DisplayItem::Full(*item);

        std::cout << IDN << "Select an entry: ";
        auto location = Input::integer();

        if (location < 1 || location > mem_count)
        {
            std::cerr << "\n [ERROR] * Invalid choice *" << '\n';
            return InvActionResult::Failed;
        }
        --location;

        std::cout << "\n";

        auto entry = item->allocated_to;
        for (int i = 0; i < location; ++i)
            entry = entry->next;

        /* TODO: check for unreachable case of entry->borrow_count == 0 */
        Core::Retrieve(item, entry);

        std::cout << "Item \"" << item->meta.name << "\" retrieved successfully\n";

        return InvActionResult::Ok;
    }

    InvActionResult ItemDetails(Inventory& inv)
    {
        SELECT_ITEM(inv, item);

        std::cout << "\n";

        DisplayItem::Full(*item);

        return InvActionResult::Ok;
    }
}; // namespace Frontend

namespace Core
{

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
            if (head->name == name)
                return head;

            head = head->next;
        }

        return nullptr;
    }

    static void Add(Inventory& inv, item_id_t id, item_count_t icount, const ItemMeta& meta)
    {
        if (inv.count == inv.capacity)
            inventory_allocate_capacity(inv, grow(inv.capacity));

        InventoryItem& slot = inv.items[inv.count++];

        slot.item_id = id;

        slot.meta = meta;

        slot.item_count = icount;
        slot.allocated_to = nullptr;
    }

    static inline void Delete(InventoryItem* item)
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

            Member** head = first == nullptr ? &item->allocated_to : &first->next;

            *head = second;

            if (second != nullptr)
                second->prev = first;

            DeleteMember(entry);
        }

        ++item->item_count;
        --item->assigned_count;
    }
} // namespace Core
