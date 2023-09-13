#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <utility>
#include <tuple>
#include <functional>

class Research
{
    public:
        Research(int researchPoints, const int maxResearchPoints) : mResearchPoints(researchPoints), mMaxResearchPoints(maxResearchPoints)
        {}

        std::function<void()> GetModifySpeedFunction(int newSpeed)
        {
            return [this, newSpeed]() { ModifyResearchSpeed(newSpeed); };
        }

        void IncrementResearchPoints(std::mutex& mut);
        void PurchaseUpgrades();

        void AddUpgrade(const std::string& upgradeName, int rpCost, std::function<void()> func)
        {
            mUpgrades.push_back(std::make_tuple(upgradeName, rpCost, func));
        }

        void ViewUpgradeList()
        {
            for (int counter{ 0 }; const auto& [upgradeName, cost, _] : mUpgrades)
            {
                std::cout << ++counter << "). " << upgradeName << " - RP" << cost << "\n";
            }
        }

        int GetResearchPoints() const
        {
            return mResearchPoints;
        }

        int GetMaxResearchPoints() const
        {
            return mMaxResearchPoints;
        }

    private:
        int mResearchPoints{ 0 };
        const int mMaxResearchPoints{ 5000 };
        int mResearchSpeed{ 1000 };

        std::vector<std::tuple<std::string, int, std::function<void()>>> mUpgrades{};

        void ModifyResearchSpeed(int newSpeed)
        {
            mResearchSpeed = newSpeed;
        }
};

int main()
{
    Research research(400, 5000);
    //std::function<void()> computingPowerOne = [&research]() { research.ModifyResearchSpeed(500); };
    auto computingPowerOne = research.GetModifySpeedFunction(500);
    research.AddUpgrade("Computing Power - Makes gaining Research Points faster.", 13, computingPowerOne);

    //std::function<void()> computingPowerTwo = [&research]() { research.ModifyResearchSpeed(300); };
    auto computingPowerTwo = research.GetModifySpeedFunction(300);
    research.AddUpgrade("Computing Power 2 - Makes gaining Research Points even faster.", 26, computingPowerTwo);

    std::mutex mut;

    //We want our research points to accumulate in the background so we put it on a separate thread.
    std::jthread researchThread(&Research::IncrementResearchPoints, &research, std::ref(mut));

    int input{ 0 };
    bool runGame{ true };

    while (runGame)
    {
        std::cout << "What would you like to do?\n\n";
        std::cout << "1) View Available Research Points.\n";
        std::cout << "2) Purchase Upgrade\n";

        std::cin >> input;

        switch (input)
        {
            case 1:
                {
                    std::lock_guard<std::mutex> lock(mut);
                    std::cout << "Current Research Points: " << research.GetResearchPoints() << '\n';
                }
                break;
            case 2:
                research.PurchaseUpgrades();
                break;
            default:
                std::cout << "Invalid Choice.\n";
        }
    }
}

void Research::IncrementResearchPoints(std::mutex& mut)
{
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            if (mResearchPoints < mMaxResearchPoints)
            {
                ++mResearchPoints;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(mResearchSpeed));
    }
}

void Research::PurchaseUpgrades()
{
    std::cout << "Which upgrade would you like to purchase?\n\n";

    std::cout << "Current RP: " << mResearchPoints << "\n\n";

    ViewUpgradeList();
    
    std::cout << '\n';
    
    int choice{ 0 };
    std::cout << '>';
    std::cin >> choice;

    if (mResearchPoints >= std::get<1>(mUpgrades[choice -1]))
    {
        std::cout << "You purchased the " << std::get<0>(mUpgrades[choice - 1]) << " for " << std::get<1>(mUpgrades[choice - 1]) << " RP\n";
        mResearchPoints -= std::get<1>(mUpgrades[choice - 1]);
        std::cout << "Remaining RP: " << mResearchPoints << '\n';

        std::get<2>(mUpgrades[choice - 1])();
    }
    else
    {
        std::cout << "You do not have enough Research Points to purchase " << std::get<0>(mUpgrades[choice - 1]) << '\n';
    }
}