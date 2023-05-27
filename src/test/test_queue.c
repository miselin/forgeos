#include <gtest/gtest.h>

#include "../kernel/include/util.h"

TEST(QueueTest, CreateDelete) {
    void *q = create_queue();
    delete_queue(q);
}
