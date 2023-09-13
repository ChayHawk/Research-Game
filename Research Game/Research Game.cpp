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

        void AddUpgrade(const std::string& upgradeName, int rpCost, bool hasUpgrade, std::function<void()> func)
        {
            mUpgrades.push_back(std::make_tuple(upgradeName, rpCost, hasUpgrade, func));
        }

        void ViewUpgradeList()
        {
            for (int counter{ 0 }; const auto& [upgradeName, cost, hasUpgrade, _] : mUpgrades)
            {
                std::cout << ++counter << "). " << upgradeName;

                if (hasUpgrade == true)
                {
                    std::cout << "[Owned]" << " - RP" << cost << "\n";
                }
                else
                {
                   std::cout << " - RP" << cost << "\n";
                }
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

        std::vector<std::tuple<std::string, int, bool, std::function<void()>>> mUpgrades{};

        void ModifyResearchSpeed(int newSpeed)
        {
            mResearchSpeed = newSpeed;
        }
};

int main()
{
    Research research(400, 5000);

    std::function<void()> computingPowerOne = research.GetModifySpeedFunction(500);
    research.AddUpgrade("Computing Power", 13, false, computingPowerOne);

    std::function<void()> computingPowerTwo = research.GetModifySpeedFunction(300);
    research.AddUpgrade("Computing Power 2", 26, false, computingPowerTwo);

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

    int indexShift{ choice - 1 };

    if (mResearchPoints >= std::get<1>(mUpgrades[indexShift]))
    {
        if (std::get<2>(mUpgrades[indexShift]) == true)
        {
            std::cout << "You already have this upgrade, you cannot buy it again!\n\n";
        }
        else 
        {
            std::cout << "You purchased the " << std::get<0>(mUpgrades[indexShift]) << " for " << std::get<1>(mUpgrades[indexShift]) << " RP\n";
            mResearchPoints -= std::get<1>(mUpgrades[indexShift]);
            std::cout << "Remaining RP: " << mResearchPoints << '\n';

            std::get<2>(mUpgrades[indexShift]) = true;

            //Get our upgrade
            std::get<3>(mUpgrades[indexShift])();
        }
    }
    else
    {
        std::cout << "You do not have enough Research Points to purchase " << std::get<0>(mUpgrades[indexShift]) << '\n';
    }
}