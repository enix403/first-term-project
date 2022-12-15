#pragma once

#ifndef __APP_REPR_H_
#define __APP_REPR_H_

#include <cstdint>

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

static constexpr std::size_t MAX_NAME_LEN = 24;

typedef char name_str_t[MAX_NAME_LEN];

struct Member
{
    char name[MAX_NAME_LEN];
    int borrow_count = 0;

    Member* prev = nullptr;
    Member* next = nullptr;
};

typedef uint16_t item_id_t;
typedef uint32_t item_count_t;

struct ItemMeta
{
    char name[MAX_NAME_LEN];
    ItemCategory cat;
};

struct InventoryItem
{
    item_id_t item_id = 0;
    ItemMeta meta {};
    item_count_t item_count = 0;
    item_count_t assigned_count = 0;
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

#endif
