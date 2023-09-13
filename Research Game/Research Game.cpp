#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <utility>
#include <tuple>
#include <functional>
#include <limits>

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

        //jthread was not terminating the thread no matter what I did. I tried to
        // return 0 from 3 in the switch statement and the program just hung, so
        // I passed a stop_token to the functions and the thread and that didnt work.
        //this solution works though. Not sure if this causes any ill effects
        //on the thread though.
        void TerminateLoop()
        {
            endResearchLoop = true;
        }

    private:
        int mResearchPoints{ 0 };
        const int mMaxResearchPoints{ 5000 };
        int mResearchSpeed{ 1000 };

        std::vector<std::tuple<std::string, int, bool, std::function<void()>>> mUpgrades{};

        bool endResearchLoop{ false };

        void ModifyResearchSpeed(int newSpeed)
        {
            mResearchSpeed = newSpeed;
        }
};

int main()
{
    Research research(0, 50);

    std::function<void()> computingPowerOne = research.GetModifySpeedFunction(500);
    research.AddUpgrade("Computing Power 1", 13, false, computingPowerOne);

    std::function<void()> computingPowerTwo = research.GetModifySpeedFunction(300);
    research.AddUpgrade("Computing Power 2", 26, false, computingPowerTwo);

    std::function<void()> computingPowerThree = research.GetModifySpeedFunction(100);
    research.AddUpgrade("Computing Power 3", 50, false, computingPowerThree);

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
        std::cout << "3) Quit\n";

        std::cin >> input;

        if (input > 3)
        {
            std::cout << "\n[ERROR] There are only 3 choices! Please try again\n";
        }
        else if (std::cin.fail())
        {
            std::cout << "\n[ERROR] Non integer value entered, please try again.\n\n";
        }
        else
        {
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
            case 3:
                research.TerminateLoop();
                runGame = false;
                break;
            default:
                std::cout << "Invalid Input\n";
            }
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    return 0;
}

void Research::IncrementResearchPoints(std::mutex& mut)
{
    while (!endResearchLoop)
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
    std::cout << "\nWhich upgrade would you like to purchase?\n\n";

    std::cout << "Current RP: " << GetResearchPoints() << "\n\n";

    ViewUpgradeList();
    
    std::cout << '\n';
    
    int choice{ 0 };
    std::cout << '>';
    std::cin >> choice;

    if (choice > mUpgrades.size())
    {
        std::cout << "\n[ERROR] There are only " << mUpgrades.size() <<  " choices! Please try again\n";
    }
    else if (std::cin.fail())
    {
        std::cout << "\n[ERROR] Non integer value entered, please try again.\n\n";
    }
    else
    {
        int shiftIndex{ choice - 1 };

        if (mResearchPoints >= std::get<1>(mUpgrades[shiftIndex]))
        {
            if (std::get<2>(mUpgrades[shiftIndex]) == true)
            {
                std::cout << "You already have this upgrade, you cannot buy it again!\n\n";
            }
            else
            {
                std::cout << "You purchased the " << std::get<0>(mUpgrades[shiftIndex]) << " for " << std::get<1>(mUpgrades[shiftIndex]) << " RP\n";

                mResearchPoints -= std::get<1>(mUpgrades[shiftIndex]);

                std::cout << "Remaining RP: " << GetResearchPoints() << '\n';

                std::get<2>(mUpgrades[shiftIndex]) = true;

                //Get our upgrade
                std::get<3>(mUpgrades[shiftIndex])();
            }
        }
        else
        {
            std::cout << "You do not have enough Research Points to purchase " << std::get<0>(mUpgrades[shiftIndex]) << '\n';
        }
    }

    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}