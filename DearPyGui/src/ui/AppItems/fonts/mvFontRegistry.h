#pragma once

#include "mvItemRegistry.h"

namespace Marvel {

    class mvFontRegistry : public mvAppItem
    {

    public:

        explicit mvFontRegistry(mvUUID uuid);

        void draw(ImDrawList* drawlist, float x, float y) override;
        void customAction(void* data = nullptr) override;
        void onChildAdd(mvRef<mvAppItem> item) override { _show = true; }

        bool isInvalid() const { return _dirty; }
        void resetFont();

    private:

        bool _dirty = true;

    };

}
