#pragma once

#include <array>
#include "mvItemRegistry.h"
#include <imnodes.h>

namespace Marvel {

    class mvThemeColor : public mvAppItem
    {

    public:

        explicit mvThemeColor(mvUUID uuid);

        void draw(ImDrawList* drawlist, float x, float y) override;
        void customAction(void* data = nullptr) override;
        void handleSpecificPositionalArgs(PyObject* dict) override;
        void handleSpecificKeywordArgs(PyObject* dict) override;
        void getSpecificConfiguration(PyObject* dict) override;
        void applySpecificTemplate(mvAppItem* item) override;
        
        // values
        void setDataSource(mvUUID dataSource) override;
        void* getValue() override { return &_value; }
        PyObject* getPyValue() override;
        void setPyValue(PyObject* value) override;
        
        void setLibType(mvLibType libType) { _libType = libType; }

    private:

        mvRef<std::array<float, 4>> _value = CreateRef<std::array<float, 4>>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        ImGuiCol _targetColor = 0;
        mvLibType _libType = mvLibType::MV_IMGUI;
        
    };

}
