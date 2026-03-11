#include <coroutine>
#include <exception>
#include <vector>
#include <set>
#include <cstdint>
#include <iostream>
#include <cassert>  

struct task {
	struct promise_type {
        uint32_t node_{};

    	task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

    	std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

        std::suspend_always yield_value(uint32_t node)
        {
            node_ = node;
            return {};
        }

        void return_void() {}
	};

    explicit task(std::coroutine_handle<promise_type> handle) : handle(handle) {}

    ~task() {
        if (handle) { handle.destroy(); }
    }
    
    uint32_t next() {
        if (handle) {
            handle.resume();
            return handle.promise().node_;
        }
        else {
            return {};
        }
    }

	std::coroutine_handle<promise_type> handle;
};

task dfs(const std::vector<std::set<uint32_t>>& neighbors_for_nodes, 
         const uint32_t node,
         std::vector<bool>& is_visited) {
    is_visited[node] = true;
    co_yield node;
    for (const uint32_t new_node : neighbors_for_nodes[node]) {
        if (!is_visited[new_node]) {
            task new_task = dfs(neighbors_for_nodes, new_node, is_visited);
            while (!new_task.handle.done()) {
                const uint32_t next_node = new_task.next();
                if (!new_task.handle.done()) {
                    co_yield next_node;
                }
            }
        }
    }
}

const std::vector<std::set<uint32_t>> transform_graph(const std::vector<std::pair<uint32_t, uint32_t>>& pairs_of_nodes, 
                                                      uint32_t nodes_number) {
    std::vector<std::set<uint32_t>> result(nodes_number);
    for (const auto& [from, to] : pairs_of_nodes) {
        result[from].insert(to);
        result[to].insert(from);
    }
    return result;
}

int main() {
    std::vector<std::pair<uint32_t, uint32_t>> edges = {
        {0, 1},
        {0, 2},
        {0, 3},
        {1, 4},
        {2, 6},
        {4, 5},
        {6, 7}
    };
    const uint32_t nodes_number = 8;
    std::vector<std::set<uint32_t>> neighbors_for_nodes = transform_graph(edges, nodes_number);
    std::vector<bool> is_visited1(nodes_number, 0);
    std::vector<bool> is_visited2(nodes_number, 0);
    task result1 = dfs(neighbors_for_nodes, 0, is_visited1);
    task result2 = dfs(neighbors_for_nodes, 6, is_visited2);
    std::vector<uint32_t> node_order1;
    node_order1.reserve(nodes_number);
    std::vector<uint32_t> node_order2;
    node_order2.reserve(nodes_number);
    while (!result1.handle.done() || !result2.handle.done()) {
        uint32_t node = result1.next();
        if (!result1.handle.done()) {
            node_order1.push_back(node);
            std::cout << "1: " << node << std::endl;
        }

        node = result2.next();
        if (!result2.handle.done()) {
            node_order2.push_back(node);
            std::cout << "2: " << node << std::endl;
        }
    }
    assert(node_order1.size() == nodes_number);
    assert(node_order2.size() == nodes_number);
    std::vector<uint32_t> correct_node_order1{0, 1, 4, 5, 2, 6, 7, 3};
    std::vector<uint32_t> correct_node_order2{6, 2, 0, 1, 4, 5, 3, 7};
    assert(correct_node_order1 == node_order1);
    assert(correct_node_order2 == node_order2);
    return 0;
}