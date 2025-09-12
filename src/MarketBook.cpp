#include "MarketBook.h"

MarketBook::MarketBook(HashTables &hashtables) noexcept : hashtables_(hashtables) {}

void MarketBook::add_order(const Order &order) noexcept
{
   auto &symBook = get_symBook(order);
   auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

   auto it = tree.find(order.price);
   if (it == tree.end())
      tree.emplace(order.price, order.quantity);
   else
      it->second += order.quantity;
}

void MarketBook::modify_order(const Order &order, uint32_t newQuantity) noexcept
{
   auto &symBook = get_symBook(order);
   auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

   int64_t diff = static_cast<int64_t>(newQuantity) - static_cast<int64_t>(order.quantity);

   auto it = tree.find(order.price);
   if (it != tree.end())
   {
      it->second += diff;
      if (it->second <= 0)
         tree.erase(it);
   }
   else
   {
      if (diff > 0)
         tree[order.price] = static_cast<uint32_t>(diff);
   }
}

void MarketBook::cancel_order(const Order &order) noexcept
{
   auto &symBook = get_symBook(order);
   auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

   auto it = tree.find(order.price);
   if (LIKELY(it != tree.end()))
   {
      if (it->second <= order.cancelled_quantity)
         tree.erase(it);
      else
         it->second -= order.cancelled_quantity;
   }
}

void MarketBook::delete_order(const Order &order) noexcept
{
   auto &symBook = get_symBook(order);
   auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

   uint32_t remaining_qty = order.quantity - order.filled_quantity;

   auto it = tree.find(order.price);
   if (LIKELY(it != tree.end()))
   {
      if (it->second <= remaining_qty)
         tree.erase(it);
      else
         it->second -= remaining_qty;
   }
}

void MarketBook::trade_order(const Order &order) noexcept
{
   // Partial fill -> filled qty kadar azalt
   auto &symBook = get_symBook(order);
   auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

   auto it = tree.find(order.price);
   if (LIKELY(it != tree.end()))
   {
      if (it->second <= order.last_exec_quantity)
         tree.erase(it);
      else
         it->second -= order.last_exec_quantity;
   }
}