#pragma once

#include "Fragment.h"
#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <memory>

namespace tmms
{
    namespace mm
    {
        using std::string;
        using FragmentPtr = std::shared_ptr<Fragment>;
        class FragmentWindow
        {
        public:
            FragmentWindow(int32_t size = 5);
            ~FragmentWindow();

            void AppendFragment(FragmentPtr &&fragment);
            FragmentPtr GetIdleFragment();
            const FragmentPtr &GetFragmentByName(const string &name);
            string GetPlayList();
            
        private:
            void Shrink();
            void UpdatePlayList();

            int32_t window_size_{5};
            std::vector<FragmentPtr> fragments_;
            std::vector<FragmentPtr> free_fragments_;
            std::string playlist_;
            std::mutex lock_;
        };
    }
}