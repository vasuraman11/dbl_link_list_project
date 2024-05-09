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
  void confirm_empty()
  {
    REQUIRE((!head && !tail));
  }
  void check_list_integrity(int expected_nodes)
  {
    std::shared_lock l(guard);
    REQUIRE(head);
    REQUIRE(tail);
    int nodes{0};
    for (node *n = head; n; n = n->next) {
      nodes++;
      REQUIRE(n->app_data);
      REQUIRE((n == head || n->previous));
      REQUIRE((n == tail || n->next));
    }
    REQUIRE(nodes == expected_nodes);
  }
  void confirm_found(void *app_data, int expected_found) {
    std::shared_lock l(guard);
    REQUIRE(head);
    REQUIRE(tail);
    int found{0};
    for (node *n = head; n; n = n->next) {
      if (app_data == n->app_data) { found++; }
    }
    REQUIRE(found == expected_found);
  }
};

TEST_CASE("Nodes can be added and removed", "[basic_test]")
{
  std::string app_data1{"data1"};
  std::string app_data2{"data2"};
  std::string app_data3{"data3"};

  TEST_DBL_LINK_LIST list;

  SECTION("Adding some nodes") {
    list.confirm_empty();
    REQUIRE(list.add(&app_data1) == err_code::SUCCESS);
    list.check_list_integrity(1);
    list.confirm_found(&app_data1, 1);

    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    list.check_list_integrity(2);
    list.confirm_found(&app_data1, 1);
    list.confirm_found(&app_data2, 1);

    REQUIRE(list.add(&app_data3) == err_code::SUCCESS);
    list.check_list_integrity(3);
    list.confirm_found(&app_data1, 1);
    list.confirm_found(&app_data2, 1);
    list.confirm_found(&app_data3, 1);

    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    list.check_list_integrity(4);
    list.confirm_found(&app_data1, 1);
    list.confirm_found(&app_data2, 2);
    list.confirm_found(&app_data3, 1);
  }

  SECTION("Removing nodes") (
    list.confirm_empty();
    REQUIRE(list.add(&app_data1) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data3) == err_code::SUCCESS);
    REQUIRE(list.add(&app_data2) == err_code::SUCCESS);
    list.check_list_integrity(4);

    node *n = list.get_node(2);
    REQUIRE(list.remove())
  )
}