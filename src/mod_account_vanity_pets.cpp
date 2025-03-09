#include "Chat.h"
#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <set>         // Required for std::set             -- might be redundant
#include <sstream>     // Required for std::istringstream   -- might be redundant
#include <string>      // Required for std::string          -- might be redundant

class AccountVanityPets : public PlayerScript
{
    static const bool limitrace = true; // This set to true will only learn mounts from chars on the same team, do what you want.
    std::set<uint32> excludedSpellIds; // Set to hold the Spell IDs to be excluded

public:
    AccountVanityPets() : PlayerScript("AccountVanityPets", {
        PLAYERHOOK_ON_LOGIN
    })
    {
        // Retrieve the string of excluded Spell IDs from the config file
        std::string excludedSpellsStr = sConfigMgr->GetOption<std::string>("Account.VanityPets.ExcludedSpellIDs", "");
        // Proceed only if the configuration is not "0" or empty, indicating exclusions are specified
        if (excludedSpellsStr != "0" && !excludedSpellsStr.empty())
        {
            std::istringstream spellStream(excludedSpellsStr);
            std::string spellIdStr;
            while (std::getline(spellStream, spellIdStr, ','))
            {
                uint32 spellId = static_cast<uint32>(std::stoul(spellIdStr));
                if (spellId != 0) // Ensure the spell ID is not 0, as 0 is used to indicate no exclusions
                    excludedSpellIds.insert(spellId); // Add the Spell ID to the set of exclusions
            }
        }
    }
    
    void OnPlayerLogin(Player* pPlayer)
    {
        if (sConfigMgr->GetOption<bool>("Account.VanityPets.Enable", true))
        {
            if (sConfigMgr->GetOption<bool>("Account.VanityPets.Announce", false))
                ChatHandler(pPlayer->GetSession()).SendSysMessage("This server is running the |cff4CFF00AccountVanityPets |rmodule.");

            std::vector<uint32> Guids;
            uint32 playerAccountID = pPlayer->GetSession()->GetAccountId();
            QueryResult result1 = CharacterDatabase.Query("SELECT `guid` FROM `characters` WHERE `account`={};", playerAccountID);

            if (!result1)
                return;

            do
            {
                Field* fields = result1->Fetch();
                Guids.push_back(fields[0].Get<uint32>());

            } while (result1->NextRow());

            std::vector<uint32> Spells;

            for (auto& i : Guids)
            {
                QueryResult result2 = CharacterDatabase.Query(" SELECT cs.spell from character_spell cs LEFT JOIN acore_world.item_template it ON it.spellid_2 = cs.spell WHERE cs.guid = {} AND ((it.class = 15 AND it.subclass = 2) OR it.BagFamily = 4096);", i);
                if (!result2)
                    continue;

                do
                {
                    Spells.push_back(result2->Fetch()[0].Get<uint32>());
                } while (result2->NextRow());
            }

            for (auto& i : Spells)
            {
                // Check if the spell is in the excluded list before learning it
                if (excludedSpellIds.find(i) == excludedSpellIds.end())
                {
                    auto sSpell = sSpellStore.LookupEntry(i);
                    pPlayer->learnSpell(sSpell->Id);
                }
            }
        }
    }
};

void AddAccountVanityPetsScripts()
{
    new AccountVanityPets();
}
