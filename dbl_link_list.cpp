#include <string>
#include <iostream>
#include <shared_mutex>
#include "catch_amalgamated.hpp"

enum class err_code{ SUCCESS, ERROR };

class node {
public:
  node() {}
  node(void *app_data) : app_data{app_data} {}
  ~node() = default;
  void *get_app_data() const { return app_data; }
  friend class dbl_link_list;
  friend class TEST_DBL_LINK_LIST;

private:
  node *next{nullptr};
  node *previous{nullptr};
  void *app_data{nullptr};
};

class dbl_link_list {
public:
  err_code add(void *app_data)
  {
    if (!app_data) { return err_code::ERROR; }
    if (node *n = new node(app_data); n) {
      std::unique_lock l(guard);
      if (!head && !tail) {
        head = n;
        tail = n;
        return err_code::SUCCESS;
      }
      else if (head && tail) {
        tail->next = n;
        if (tail->next) {
          tail->next->previous = tail;
          tail = tail->next;
          return err_code::SUCCESS;
        }
      }
    }
    return err_code::ERROR;
  }
  err_code remove(node *node_to_delete)
  {
    node *n{nullptr};
    {
      std::shared_lock l(guard);
      for (n = head; n && n != node_to_delete; n = n->next);
    }
    if (n) {
      std::unique_lock l(guard);
      if (n) {
        if (n->previous) {
          n->previous->next = n->next;
        }
        else {
          head = n->next;
        }
        if (n->next) {
          n->next->previous = n->previous;
        }
        else {
          tail = n->previous;
        }
        delete n;
        return err_code::SUCCESS;
      }
    }
    return err_code::ERROR;
  }

protected:
  node *head{nullptr};
  node *tail{nullptr};
  mutable std::shared_mutex guard;
};

class TEST_DBL_LINK_LIST : public dbl_link_list {
  public:
  bool confirm_empty()
  {
    return !head && !tail;
  }
  
  bool check_list_integrity(int expected_nodes)
  {
    std::shared_lock l(guard);
    if (!head || !tail) { return false; }
    int nodes{0};
    for (node *n = head; n; n = n->next) {
      nodes++;
      if (!n->app_data) { return false; }
      if (n != head && !n->previous) { return false; }
      if (n != tail && !n->next) { return false; }
      if (!n->next) {
        if (tail != n) { return false; }
      }
    }
    if (nodes != expected_nodes) { return false; }
    return true;
  }

  int confirm_found(void *app_data) {
    std::shared_lock l(guard);
    if (!head) { return -1; }
    if (!tail) { return -1; }
    int found{0};
    for (node *n = head; n; n = n->next) {
      if (app_data == n->app_data) { found++; }
    }
    return found;
  }

  node *get_node(int position) {
    std::shared_lock l(guard);
    if (!head) { return nullptr; }
    if (!tail) { return nullptr; }
    node *n = head;
    for (int i = 0; i < position && n; i++) {
      n = n->next;
    }
    return n;
  }
};

TEST_CASE("Nodes can be added and removed", "[basic_test]")
{
  std::string app_data1{"data1"};
  std::string app_data2{"data2"};
  std::string app_data3{"data3"};

  TEST_DBL_LINK_LIST list;

  SECTION("Adding some nodes") {
    REQUIRE(list.confirm_empty());
    REQUIRE(list.add(&app_data1) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(1));
    REQUIRE(list.confirm_found(&app_data1) == 1);

    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(2));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 1);

    REQUIRE(list.add(&app_data3) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(3));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 1);

    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(4));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 2);
    REQUIRE(list.confirm_found(&app_data3) == 1);
  }

  SECTION("Removing nodes") {
    REQUIRE(list.confirm_empty());
    REQUIRE(!list.get_node(0));

    // 1 -> 2 -> 3 -> 2
    REQUIRE(list.add(&app_data1) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data3) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(4));

    // 1 -> 3 -> 2
    node *n = list.get_node(1);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(3));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 1);

    // 1 -> 2
    n = list.get_node(1);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(2));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 0);

    // out of bounds remove
    n = list.get_node(3);
    REQUIRE(!n);

    // 2
    n = list.get_node(0);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(1));
    REQUIRE(list.confirm_found(&app_data1) == 0);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 0);

    // 2 -> 1
    REQUIRE(list.add(&app_data1) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(2));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 0);

    // 2
    n = list.get_node(1);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(1));
    REQUIRE(list.confirm_found(&app_data1) == 0);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 0);

    // empty list
    n = list.get_node(0);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.confirm_empty());
    REQUIRE(list.confirm_found(&app_data2) == -1);

    // try to remove on empty list
    REQUIRE(list.remove(n) == err_code::ERROR);

    // mix stuff and make sure list is ok
    REQUIRE(list.add(&app_data1) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    n = list.get_node(1);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data3) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(4));
    n = list.get_node(3);
    REQUIRE(n);
    REQUIRE(list.remove(n) == err_code::SUCCESS);
    REQUIRE(list.check_list_integrity(3));
    REQUIRE(list.confirm_found(&app_data1) == 1);
    REQUIRE(list.confirm_found(&app_data2) == 1);
    REQUIRE(list.confirm_found(&app_data3) == 1);
 }
}