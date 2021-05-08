#pragma once

extern "C" {
struct mgb;
}

namespace menu {

struct Button {
    const char* const title;
    void (*callback)(struct mgb* mgb);
};

void Main(struct mgb* mgb);

} // namespace menu