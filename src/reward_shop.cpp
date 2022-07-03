/*

Database Actions:

1 = item
2 = gold
3 = name change
4 = faction change
5 = race change

script made by talamortis

*/

#include "Configuration/Config.h"
#include "Player.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Chat.h"

class reward_shop : public CreatureScript
{
public:
    reward_shop() : CreatureScript("reward_shop") {}

    bool failedcode;

    bool OnGossipHello(Player *player, Creature *creature)
    {
        if (player->IsInCombat())
            return false;

        if (!sConfigMgr->GetOption<bool>("RewardShopEnable", 0))
            return false;

        std::string text = "粘贴或者输入兑换码并点击确认完成礼品兑换";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我有兑换码要兑换", GOSSIP_SENDER_MAIN, 1, text, 0, true);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "兑换码怎么来?", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "再见!", GOSSIP_SENDER_MAIN, 3);

        if (sConfigMgr->GetOption<bool>("AllowGM", 1) && player->IsGameMaster())
        {
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "[GM] I would like generate a code.", GOSSIP_SENDER_MAIN, 4);
        }

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 /* sender */, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        std::string info = sConfigMgr->GetOption<std::string>("WebsiteAddress", "You can get codes by visiting the online store at (website address)");
        uint32 rnd1 = urand(10000, 90000);
        uint32 rnd2 = urand(10000, 90000);
        uint32 rnd3 = urand(10000, 90000);
        uint32 rnd4 = urand(10000, 90000);
        uint32 rnd5 = urand(10000, 90000);

        std::string CreatedBy = player->GetName();
        std::ostringstream randomcode;
        randomcode << "GM-" << rnd1 << "-" << rnd2 << "-" << rnd3 << "-" << rnd4 << "-" << rnd5;
        uint32 GiftBuff = sConfigMgr->GetOption<uint32>("GiftBuff", 47292);

        switch (action)
        {
        case 2:
            creature->Whisper(info.c_str(), LANG_UNIVERSAL, player);
            CloseGossipMenuFor(player);
            break;
        case 3:
            CloseGossipMenuFor(player);
            creature->CastSpell(player,GiftBuff,true);
            break;
        case 4:
            player->PlayerTalkClass->ClearMenus();
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I'd like to generate a name change code.", GOSSIP_SENDER_MAIN, 6);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I'd like to generate a faction change code.", GOSSIP_SENDER_MAIN, 7);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I'd like to generate a race change code.", GOSSIP_SENDER_MAIN, 8);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 6:
            CharacterDatabase.Query("INSERT INTO `reward_shop` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`, `CreatedBy`) VALUES(3, 0, 0, '%s', 0, 0, '0', '%s')", randomcode.str().c_str(), CreatedBy.c_str());
            ChatHandler(player->GetSession()).PSendSysMessage("Code was successfully created your code is %s", randomcode.str().c_str());
            break;
        case 7:
            CharacterDatabase.Query("INSERT INTO `reward_shop` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`,`CreatedBy`) VALUES(4, 0, 0, '%s', 0, 0, '0', '%s')", randomcode.str().c_str(), CreatedBy.c_str());
            ChatHandler(player->GetSession()).PSendSysMessage("Code was successfully created your code is %s", randomcode.str().c_str());
            break;
        case 8:
            CharacterDatabase.Query("INSERT INTO `reward_shop` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`, `CreatedBy`) VALUES(5, 0, 0, '%s', 0, 0, '0', '%s')", randomcode.str().c_str(), CreatedBy.c_str());
            ChatHandler(player->GetSession()).PSendSysMessage("Code was successfully created your code is %s", randomcode.str().c_str());
            break;
        }
        return true;
    }

    bool OnGossipSelectCode(Player *player, Creature *creature, uint32 /* sender */, uint32, const char *code)
    {
        ObjectGuid playerguid = player->GetGUID();
        std::string playerIP = player->GetSession()->GetRemoteAddress();
        std::string rewardcode = code;
        std::ostringstream message_invalide;
        std::ostringstream message_used;
        message_invalide << "这样," << player->GetName() << ", 这个码：" << rewardcode << " 是不是输入错误了？，如果你确定输入的是正规无误的兑换码，那么请截图并发送给群管理报告一下，以便查明原因";
    
        std::size_t found = rewardcode.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-");

        if (found != std::string::npos)
            return false;

        // check for code
        QueryResult result = CharacterDatabase.Query("SELECT id, action, action_data, quantity, status, PlayerGUID FROM reward_shop WHERE code = '%s'", rewardcode.c_str());

        if (!result)
        {
            player->PlayDirectSound(9638); // No
            creature->Whisper(message_invalide.str().c_str(), LANG_UNIVERSAL, player);
            creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return false;
        }

        std::transform(rewardcode.begin(), rewardcode.end(), rewardcode.begin(), ::toupper);

        do
        {
            Field *fields = result->Fetch();
            uint32 action = fields[1].Get<uint32>();
            uint32 action_data = fields[2].Get<uint32>();
            uint32 quantity = fields[3].Get<uint32>();
            uint32 status = fields[4].Get<uint32>();
            int count = 1;
            uint32 noSpaceForCount = 0;
            ItemPosCountVec dest;
            InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, action_data, quantity, &noSpaceForCount);

            if (status == 1)
            {
                uint32 user = fields[5].Get<uint32>();
                message_used << "这样," << player->GetName() << ", 这个码：" << rewardcode << " 已经兑换过了，兑换角色ID：" << user << " 如果你确定输入的是正规无误的兑换码，那么请截图并发送给群管理报告一下，以便查明原因";
                player->PlayDirectSound(9638); // No
                creature->Whisper(message_used.str().c_str(), LANG_UNIVERSAL, player);
                creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
                return false;
            }
            switch (action)
            {

            case 1: /* Item */
                if (msg != EQUIP_ERR_OK)
                    count -= noSpaceForCount;

                if (count == 0 || dest.empty())
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("包里没空间了还是你已经有了这么一个只能唯一的东西了？");
                    ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                    return false;
                }

                if (count > 0 && action_data)
                {
                    player->AddItem(action_data, quantity);
                }
                break;
            case 2: /* Gold */
                player->ModifyMoney(action_data * 10000);
                ChatHandler(player->GetSession()).PSendSysMessage("请查收 [%u G]", action_data);
                break;
            case 3: /* Name Change */
                player->SetAtLoginFlag(AT_LOGIN_RENAME);
                ChatHandler(player->GetSession()).PSendSysMessage("CHAT OUTPUT: Please log out for name change.");
                break;
            case 4: /* Faction Change */
                player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
                ChatHandler(player->GetSession()).PSendSysMessage("CHAT OUTPUT: Please log out for faction change.");
                break;
            case 5: /* Race Change */
                player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
                ChatHandler(player->GetSession()).PSendSysMessage("CHAT OUTPUT: Please log out for race change.");
                break;
            }

        } while (result->NextRow());

        CharacterDatabase.Execute("UPDATE reward_shop SET status = 1, PlayerGUID = '%u', PlayerIP = '%s' WHERE code = '%s'", playerguid.GetCounter(), playerIP.c_str(), rewardcode.c_str());
        return true;
    }

    struct npc_reward_shopAI : public ScriptedAI
    {
        npc_reward_shopAI(Creature *creature) : ScriptedAI(creature) {}
        uint32 say_timer;
        bool canSay;
        std::string Says = sConfigMgr->GetOption<std::string>("Says", "礼品优惠，游戏更轻松，需要吗？");
        void Reset()
        {
            say_timer = 1000;
            canSay = false;
        }

        void MoveInLineOfSight(Unit *who)
        {
            if (me->IsWithinDist(who, 5.0f) && who->GetTypeId() == TYPEID_PLAYER)
            {
                canSay = true;
            }
            else
                canSay = false;
        }

        void UpdateAI(uint32 diff)
        {
            if (say_timer <= diff)
            {
                if (canSay)
                {
                    me->Say(Says, LANG_UNIVERSAL);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                    say_timer = 61000;
                }
            }
            else
                say_timer -= diff;
        }
    };
    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_reward_shopAI(creature);
    }
};

void AddRewardShopScripts()
{
    new reward_shop();
}
