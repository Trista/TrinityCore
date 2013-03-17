/*
* Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* Script Data Start
   SDName: Boss Hadronox
   Script Data End */

/*
* When your group enters the main room (the one after the bridge), you will notice a group of 3 Nerubians.
* When you engage them, 2 more groups like this one spawn behind the first one - it is important to pull the first group back,
* so you don't aggro all 3. Hadronox will be under you, fighting Nerubians.
*
* This is the timed gauntlet - waves of non-elite spiders
* will spawn from the 3 doors located a little above the main room, and will then head down to fight Hadronox. After clearing the
* main room, it is recommended to just stay in it, kill the occasional non-elites that will attack you instead of the boss, and wait for
* Hadronox to make his way to you. When Hadronox enters the main room, he will web the doors, and no more non-elites will spawn.
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "SpellScript.h"
#include "azjol_nerub.h"
#include "Player.h"
#include "CreatureTextMgr.h"
#include "TemporarySummon.h"

enum Events
{
    // =========== HADRONOX ===========
    EVENT_ACID_CLOUD           = 1,
    EVENT_LEECH_POISON	       = 2,
    EVENT_PIERCE_ARMOR	       = 3,
    EVENT_WEB_GRAB	           = 4,
    EVENT_MOVE_IN_TUNNEL       = 5,
    EVENT_TIMED_ATTACKERS_A    = 6, // Used to control periodic attacking trash from distant front door
    EVENT_TIMED_ATTACKERS_B    = 7, // Used to control periodic attacking trash from closer front door

    // =========== ANUB`AR CRUSHER ===========
    EVENT_SMASH                = 2,

    // =========== ANUB`AR CHAMPION ===========
    EVENT_REND                 = 2,
    EVENT_PUMMEL               = 3,

    // =========== ANUB`AR NECROMANCER ===========
    EVENT_SHADOW_BOLT          = 2,
    EVENT_ANIMATED_BONNES      = 3,

    // =========== CRYPT FIEND ===========
    EVENT_INFECTED_WOUNDS      = 2,
    EVENT_CRUSHING_WEBS        = 3
};

enum Actions
{
    ACTION_RESET_EVENT
};

enum Movements
{
    // Point ID used by each member of crusher reinforcement group to finalize wps
    POINT_FINALIZE_WPS       = 0
};

enum Spells
{
    // ============== HADRONOX ==============
    SPELL_ACID_CLOUD                             = 53400,
    SPELL_LEECH_POISON                           = 53030,
    SPELL_PIERCE_ARMOR                           = 53418,
    SPELL_TAUNT                                  = 53799,
    SPELL_WEB_GRAB_WITHOUT_PLAYER	             = 53406,
    SPELL_WEB_GRAB_WITH_PLAYER                   = 57731,
    SPELL_WEB_FRONT_DOORS                        = 53177,
    SPELL_WEB_SIDE_DOOR	                         = 53185,

    // ============== ANUB'AR CRUSHER ==============
    SPELL_SMASH                                  = 53318,
    SPELL_FRENZY                                 = 53801,

    // ============== ANUB'AR CHAMPION ==============
    SPELL_REND_N                                 = 53317,
    SPELL_REND_H                                 = 59343, // used as must not have aura from reference
    SPELL_PUMMEL                                 = 53394,

    // ============== ANUB'AR NECROMANCER ==============
    SPELL_ANIMATED_BONNES                        = 53334,
    SPELL_SHADOW_BOLT                            = 53333,

    // ============== ANUB'AR CRYPT FIEND ==============
    SPELL_INFECTED_WOUNDS_N                      = 53330,
    SPELL_INFECTED_WOUNDS_H                      = 59348, // used as must not have aura from reference
    SPELL_CRUSHING_WEBS                          = 53322,

    // ============== TRIGGERS ==============
    SPELL_SUMMON_ANUB_AR_CHAMPION_PERIODIC       = 53035,
    SPELL_SUMMON_ANUB_AR_NECROMANCER_PERIODIC    = 53036,
    SPELL_SUMMON_ANUB_AR_CRYPT_FIEND_PERIODIC    = 53037,
    SPELL_SUMMON_ANUB_AR_CHAMPION                = 53826, 
    SPELL_SUMMON_ANUB_AR_NECROMANCER             = 53827,
    SPELL_SUMMON_ANUB_AR_CRYPT_FIEND             = 53828
};

enum NPCs
{
    NPC_STATIC_CRUSHER               = 28922,
    NPC_STATIC_CHAMPION	             = 29117,
    NPC_STATIC_CRYPT_FIEND           = 29118,
    NPC_STATIC_NECROMANCER           = 29119,
    NPC_PERIODIC_ATTACKING_CHAMPION  = 28924,
    NPC_PERIODIC_ATTACKING_NECRO     = 28925,
    NPC_PERIODIC_ATTACKING_FIEND     = 29051,
    // Trash heading Hadronox's Lair
    NPC_ANUB_FIEND_D_A_AND_SIDE      = 29063,
    NPC_ANUB_FIEND_D_B               = 29097,
    NPC_ANUB_CHAMPION_D_A_AND_SIDE   = 29062,
    NPC_ANUB_CHAMPION_D_B            = 29096,
    NPC_ANUB_NECRO_D_A_AND_SIDE      = 29064,
    NPC_ANUB_NECRO_D_B               = 29098
};

/* There are 4 WPs, 25 secs after each reached an event starts that enables combat movement until next point is reached
   and players get warning "Hadronox moves up the tunnel!" The whole successful event takes 2 minutes. When boss reach the
   point where side doors are being webbed there is no transition and goes in move in combat enabled until reachs last doors
   unless victim is player when combat is allowed. 15 secs are left for the moving transitions periods.*/

enum Texts
{
    // Hadronox
    EMOTE_MOVE_IN_TUNNEL       = 0,

    // Anub'Ar Crusher
    YELL_BREACHED_GATE         = 0,
    EMOTE_FRENZY               = 1
};

enum HadronoxData
{
    DATA_HADRONOX_DENIED       = 0 // http://www.wowhead.com/achievement=1297
};

enum CreatureGroups
{
    CREATURE_GROUP_CRUSHER_REINFORCEMENTS_A,
    CREATURE_GROUP_CRUSHER_REINFORCEMENTS_B
};

Position const DistantDoorCrusherFinalPoint  = { 520.802f, 565.729f, 733.940f, 4.5f };
Position const DistantDoorChampionFinalPoint = { 530.965f, 563.105f, 732.844f, 4.5f };
Position const DistantDoorNecroFinalPoint    = { 511.110f, 563.919f, 734.705f, 4.5f };
Position const CloserDoorCrusherFinalPoint   = { 550.440f, 554.986f, 730.924f, 4.0f };
Position const CloserDoorFiendFinalPoint     = { 556.433f, 548.888f, 730.546f, 4.0f };
Position const CloserDoorNecroFinalPoint     = { 541.198f, 562.136f, 731.626f, 4.0f };

#define MAX_FIRST_CRUSHER_GROUP_WAYPOINTS       10
Position const FirstCrusherGroupWaypoints[MAX_FIRST_CRUSHER_GROUP_WAYPOINTS] =
{
    { 476.3005f, 620.2604f, 771.4703f, 0.0f },
    { 476.3354f, 619.2611f, 771.4703f, 0.0f },
    { 486.2941f, 611.4049f, 771.5382f, 0.0f },
    { 489.3809f, 607.9408f, 767.0921f, 0.0f },
    { 508.4963f, 586.4883f, 736.6231f, 0.0f },
    { 509.2941f, 585.5931f, 736.4778f, 0.0f },
    { 514.4525f, 579.8041f, 735.3882f, 0.0f },
    { 519.1335f, 574.5508f, 734.4075f, 0.0f },
    { 522.0310f, 571.2991f, 734.0012f, 0.0f },
    { 522.0310f, 571.2991f, 734.0012f, 0.0f }
};

#define MAX_SECOND_CRUSHER_GROUP_WAYPOINTS       10
Position const SecondCrusherGroupWaypoints[MAX_SECOND_CRUSHER_GROUP_WAYPOINTS] =
{
    { 584.8197f, 618.6154f, 771.4528f, 0.0f },
    { 583.9209f, 618.1771f, 771.4528f, 0.0f },
    { 575.4312f, 610.3569f, 771.5471f, 0.0f },
    { 546.9441f, 584.1162f, 731.4221f, 0.0f },
    { 545.5532f, 582.8351f, 730.9911f, 0.0f },
    { 540.5725f, 578.2471f, 732.3167f, 0.0f },
    { 538.4279f, 576.2716f, 732.5345f, 0.0f },
    { 536.5598f, 574.5508f, 732.5548f, 0.0f },
    { 533.9731f, 572.1681f, 732.7783f, 0.0f },
    { 533.9731f, 572.1681f, 732.7783f, 0.0f }
};

// Front distant door trash stair waypoints 
#define MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS       3
Position const FrontDoorAStairsWaypoints[MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS] =
{
    { 485.155f, 611.184f, 771.461f, 0.0f },
    { 512.667f, 588.594f, 736.528f, 0.0f },
    { 550.284f, 568.537f, 730.294f, 0.0f }
};

// Front closer door trash stair waypoints
#define MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS       3
Position const FrontDoorBStairsWaypoints[MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS] =
{
    { 576.419f, 612.233f, 771.434f, 0.0f },
    { 544.141f, 585.004f, 731.543f, 0.0f },
    { 550.284f, 568.538f, 730.294f, 0.0f }
};

// Side door trash stair waypoints
#define MAX_SIDE_DOOR_STAIRS_WAYPOINTS       2
Position const SideDoorStairsWaypoints[MAX_SIDE_DOOR_STAIRS_WAYPOINTS] =
{
    { 589.844f, 597.049f, 739.152f, 0.0f },
    { 597.902f, 587.909f, 727.186f, 0.0f }
};

// Waypoints for trash heading tunnel and Hadronox's Lair
#define MAX_TRASH_HEADING_LAIR_WAYPOINTS       11
Position const TrashHeadingLairWaypoints[MAX_TRASH_HEADING_LAIR_WAYPOINTS] =
{
    { 602.912f, 579.12f, 724.11f, 0.0f },
    { 620.422f, 561.14f, 717.12f, 0.0f },
    { 623.675f, 536.21f, 704.56f, 0.0f },
    { 617.250f, 522.58f, 695.48f, 0.0f },
    { 602.654f, 511.14f, 694.73f, 0.0f },
    { 588.071f, 511.27f, 695.12f, 0.0f },
    { 584.711f, 509.85f, 696.99f, 0.0f },
    { 566.644f, 513.36f, 698.92f, 0.0f },
    { 554.092f, 520.61f, 690.14f, 0.0f },
    { 539.741f, 533.15f, 684.36f, 0.0f },
    { 530.406f, 539.70f, 678.07f, 0.0f }
};

class HadronoxEventDespawner : public BasicEvent
{
    public:
        explicit HadronoxEventDespawner(Creature* creature) : _creature(creature)
        {
        }

        bool Execute(uint64 /*currTime*/, uint32 /*diff*/)
        {
            Trinity::CreatureWorker<HadronoxEventDespawner> worker(_creature, *this);
            _creature->VisitNearbyGridObject(300.0f, worker);
            return true;
        }

        void operator()(Creature* creature) const
        {
            switch (creature->GetEntry())
            {
                // Hadronox will respawn in 30 sec, but event triggers will
                // spawn later, so if someone fails achievement should wait
                // for their respawn timer.
                case NPC_HADRONOX:
                    creature->SetRespawnDelay(30);
                    break;
                case NPC_WORLD_TRIGGER_LARGE_AOI:
                case NPC_STATIC_CRUSHER:
                case NPC_STATIC_CHAMPION:
                case NPC_STATIC_CRYPT_FIEND:
                case NPC_STATIC_NECROMANCER:
                    return;
                    break;
                default:
                    return;
            }

            uint32 corpseDelay = creature->GetCorpseDelay();
            uint32 respawnDelay = creature->GetRespawnDelay();
            creature->SetCorpseDelay(1);

            if (CreatureData const* data = creature->GetCreatureData())
                creature->NearTeleportTo(data->posX, data->posY, data->posZ, data->orientation);

            creature->DespawnOrUnsummon();
            creature->SetCorpseDelay(corpseDelay);
        }

        private:
            Creature* _creature;
};

class boss_hadronox : public CreatureScript
{
    public:
        boss_hadronox() : CreatureScript("boss_hadronox") { }

        struct boss_hadronoxAI : public BossAI
        {
            boss_hadronoxAI(Creature* creature) : BossAI(creature, DATA_HADRONOX)
            {
            }

            void Reset()
            {
                events.Reset();

                _attackersCounterA = 0;
                _attackersCounterB = 0;

                _clearedWPMovement = false;
                _allowedAttack = true;
                _hadronoxDenied = false;
            }

            void SetData(uint32 data, uint32 value)
            {
                if (data == DATA_HADRONOX_EVENT && value == IN_PROGRESS)
                    _hadronoxDenied = true;
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_HADRONOX_DENIED)
                    return _hadronoxDenied;

                return 0;
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_RESET_EVENT:
                        me->m_Events.AddEvent(new HadronoxEventDespawner(me), me->m_Events.CalculateTime(1000));
                        instance->SetBossState(DATA_HADRONOX, NOT_STARTED);
                        break;
                    case ACTION_SUMMON_REINFORCEMENTS:
                        if (Creature* distantFrontDoorTrigger = me->GetMap()->GetCreature(instance->GetData64(DATA_FRONT_DOOR_TRIGGER_A_GUID)))
                        {
                            std::list<TempSummon*> reinforcementgroupA;
                            distantFrontDoorTrigger->SummonCreatureGroup(CREATURE_GROUP_CRUSHER_REINFORCEMENTS_A, &reinforcementgroupA);
                            for (std::list<TempSummon*>::const_iterator itr = reinforcementgroupA.begin(); itr != reinforcementgroupA.end(); ++itr)
                                (*itr)->GetAI()->DoAction(CREATURE_GROUP_CRUSHER_REINFORCEMENTS_A);

                            events.ScheduleEvent(EVENT_TIMED_ATTACKERS_A, 20*IN_MILLISECONDS);
                        }

                        if (Creature* closerFrontDoorTrigger = me->GetMap()->GetCreature(instance->GetData64(DATA_FRONT_DOOR_TRIGGER_B_GUID)))
                        {
                            std::list<TempSummon*> reinforcementgroupB;
                            closerFrontDoorTrigger->SummonCreatureGroup(CREATURE_GROUP_CRUSHER_REINFORCEMENTS_B, &reinforcementgroupB);
                            for (std::list<TempSummon*>::const_iterator itr = reinforcementgroupB.begin(); itr != reinforcementgroupB.end(); ++itr)
                                (*itr)->GetAI()->DoAction(CREATURE_GROUP_CRUSHER_REINFORCEMENTS_B);

                            events.ScheduleEvent(EVENT_TIMED_ATTACKERS_B, 1*MINUTE*IN_MILLISECONDS);
                        }

                        break;
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();

                events.ScheduleEvent(EVENT_ACID_CLOUD, urand(10, 14)*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LEECH_POISON, urand(3, 9)*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_PIERCE_ARMOR, urand(1, 3)*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WEB_GRAB, urand(15, 19)*IN_MILLISECONDS);
            }

            // When Hadronox kills any enemy (that includes a player) he will regain 10% of HP if the target had Leech Poison applied.
            void KilledUnit(Unit* Victim)
            {
                if (!Victim || !Victim->HasAura(SPELL_LEECH_POISON) || !me->isAlive())
                    return;

                me->ModifyHealth(uint32(me->CountPctFromMaxHealth(10)));
            }

            void EnterEvadeMode()
            {
                if (instance && (instance->GetData(DATA_HADRONOX_EVENT) == IN_PROGRESS))
                {
                    events.Reset();
                    me->RemoveAllAuras();
                    me->CombatStop();
                    me->ResetLootMode();
                    DoAction(ACTION_RESET_EVENT);
                }
                else
                    BossAI::EnterEvadeMode();
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type = WAYPOINT_MOTION_TYPE)
                {
                    switch (id)
                    {
                        case 4:
                            _allowedAttack = false;
                            me->AttackStop();
                            events.ScheduleEvent(EVENT_MOVE_IN_TUNNEL, 25*IN_MILLISECONDS, 2);
                            break;
                        case 6:
                            _allowedAttack = false;
                            me->AttackStop();
                            events.ScheduleEvent(EVENT_MOVE_IN_TUNNEL, 25*IN_MILLISECONDS, 2);
                            break;
                        case 8:
                            _allowedAttack = false;
                            me->AttackStop();
                            events.ScheduleEvent(EVENT_MOVE_IN_TUNNEL, 25*IN_MILLISECONDS, 2);
                            break;	
                        case 10:
                            _allowedAttack = false;
                            me->AttackStop();
                            events.ScheduleEvent(EVENT_MOVE_IN_TUNNEL, 25*IN_MILLISECONDS, 2);
                            break;	
                        case 11:
                            me->CastSpell(me, SPELL_WEB_SIDE_DOOR);
                            _hadronoxDenied = false;
                            me->AttackStop();
                            break;
                        case 12:
                            me->CastSpell(me, SPELL_WEB_FRONT_DOORS);
                            instance->SetData(DATA_HADRONOX_EVENT, DONE);
                            break;
                    }
                }
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER || who->IsControlledByPlayer())
                    return;

                if (!me->IsWithinDistInMap(who, 10.0f, false))
                    return;

                if (who->ToCreature() && who->ToCreature()->GetReactState() != REACT_DEFENSIVE)
                    return;

                DoCast(who, SPELL_TAUNT);
            }

            void UpdateAI(uint32 diff)
            {	
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (!_clearedWPMovement && SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                {
                    _clearedWPMovement = true;
                    events.CancelEventGroup(2);
                    me->GetMotionMaster()->Clear(false);
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {                      
                        case EVENT_TIMED_ATTACKERS_A:
                            if (Creature* distantFrontDoorTrigger = me->GetMap()->GetCreature(instance->GetData64(DATA_FRONT_DOOR_TRIGGER_A_GUID)))
                            {
                                switch (_attackersCounterA)
                                {
                                    case 0:
                                        distantFrontDoorTrigger->AI()->DoCastAOE(SPELL_SUMMON_ANUB_AR_NECROMANCER);
                                        ++_attackersCounterA;
                                        break;
                                    case 1: distantFrontDoorTrigger->AI()->DoCastAOE(SPELL_SUMMON_ANUB_AR_CHAMPION);
                                        ++_attackersCounterA;
                                        break;
                                    case 2: distantFrontDoorTrigger->AI()->DoCastAOE(SPELL_SUMMON_ANUB_AR_CRYPT_FIEND); break;
                                }

                                events.ScheduleEvent(EVENT_TIMED_ATTACKERS_A, 20*IN_MILLISECONDS);
                            }
                            break;
                        case EVENT_TIMED_ATTACKERS_B:
                            if (Creature* closerFrontDoorTrigger = me->GetMap()->GetCreature(instance->GetData64(DATA_FRONT_DOOR_TRIGGER_B_GUID)))
                            {
                                switch (_attackersCounterB)
                                {
                                    case 0:
                                        closerFrontDoorTrigger->AI()->DoCastAOE(SPELL_SUMMON_ANUB_AR_NECROMANCER);
                                        ++_attackersCounterB;
                                        break;
                                    case 1:
                                        closerFrontDoorTrigger->AI()->DoCastAOE(SPELL_SUMMON_ANUB_AR_CHAMPION);
                                        ++_attackersCounterB;
                                        break;
                                    case 2: closerFrontDoorTrigger->AI()->DoCastAOE(SPELL_SUMMON_ANUB_AR_CRYPT_FIEND); break;
                                }

                                events.ScheduleEvent(EVENT_TIMED_ATTACKERS_B, 20*IN_MILLISECONDS);
                            }

                            break;
                        case EVENT_ACID_CLOUD:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60, true))
                                DoCast(target, SPELL_ACID_CLOUD);

                            events.ScheduleEvent(EVENT_ACID_CLOUD, urand(20, 30)*IN_MILLISECONDS);
                            break;
                        case EVENT_LEECH_POISON:
                            DoCastAOE(SPELL_LEECH_POISON);
                            events.ScheduleEvent(EVENT_LEECH_POISON, urand(11, 14)*IN_MILLISECONDS);
                            break;
                        case EVENT_PIERCE_ARMOR:
                            DoCastVictim(SPELL_PIERCE_ARMOR);
                            events.ScheduleEvent(EVENT_PIERCE_ARMOR, 8*IN_MILLISECONDS);
                            break;
                        case EVENT_WEB_GRAB:
                            if (me->SelectNearestPlayer(60.0f))
                            {
                                DoCastAOE(SPELL_WEB_GRAB_WITHOUT_PLAYER);
                                events.ScheduleEvent(EVENT_WEB_GRAB, urand(15, 30)*IN_MILLISECONDS);
                            }
                            else
                            {
                                DoCastAOE(SPELL_WEB_GRAB_WITHOUT_PLAYER);
                                events.ScheduleEvent(EVENT_WEB_GRAB, urand(15, 30)*IN_MILLISECONDS);
                            }

                            break;
                        case EVENT_MOVE_IN_TUNNEL:
                            _allowedAttack = true;
                            sCreatureTextMgr->SendChat(me, EMOTE_MOVE_IN_TUNNEL, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_MAP);
                            break;
                    }
                }

                if (_allowedAttack)
                    DoMeleeAttackIfReady();
            }

        private:
            uint8 _attackersCounterA;  // Keep the count of periodic summoned attacker from each front door trigger /maximum of 3 for each/
            uint8 _attackersCounterB;  // -//-
            bool _disableResetChecker; // Can control reset checking mechanism with it.
            bool _allowedAttack;       // Controls if Hadronox will melee attack or not
            bool _hadronoxDenied;      // Contains data for achievement.
            bool _clearedWPMovement;   // Used to stop WP movement if player gets in Hadronox threat list.
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hadronoxAI(creature);
        }
};

class npc_anub_ar_crusher : public CreatureScript
{
    public:
        npc_anub_ar_crusher() : CreatureScript("npc_anub_ar_crusher") { }

        struct npc_anub_ar_crusherAI : public npc_escortAI
        {
            npc_anub_ar_crusherAI(Creature* creature) : npc_escortAI(creature)
            {
                _instance = creature->GetInstanceScript();
                SetDespawnAtEnd(false);
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (_instance->GetData(DATA_HADRONOX_EVENT) != IN_PROGRESS)
                {
                    Talk(YELL_BREACHED_GATE);
                    _instance->SetData(DATA_HADRONOX_EVENT, IN_PROGRESS);
                }

                _events.ScheduleEvent(EVENT_SMASH, DUNGEON_MODE(urand(4, 9)*IN_MILLISECONDS, urand(4, 7)*IN_MILLISECONDS));
            }

            void EnterEvadeMode()
            {
                _events.Reset();
                // Sorry, but that exploit with despawning event is not acceptable.
                // The reset is handled through Hadronox and Reset checker spell.
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case CREATURE_GROUP_CRUSHER_REINFORCEMENTS_A:
                        for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                            AddWaypoint(i, FirstCrusherGroupWaypoints[i].GetPositionX(), FirstCrusherGroupWaypoints[i].GetPositionY(),
                            FirstCrusherGroupWaypoints[i].GetPositionZ());

                        AddWaypoint(MAX_FIRST_CRUSHER_GROUP_WAYPOINTS, DistantDoorCrusherFinalPoint.GetPositionX(), DistantDoorCrusherFinalPoint.GetPositionY(),
                            DistantDoorCrusherFinalPoint.GetPositionZ(), 1*IN_MILLISECONDS);
                        Start(true, true);
                        break;
                    case CREATURE_GROUP_CRUSHER_REINFORCEMENTS_B:
                        for (uint8 i = 0; i < MAX_SECOND_CRUSHER_GROUP_WAYPOINTS; i++)
                            AddWaypoint(i, SecondCrusherGroupWaypoints[i].GetPositionX(), SecondCrusherGroupWaypoints[i].GetPositionY(),
                            SecondCrusherGroupWaypoints[i].GetPositionZ());

                        AddWaypoint(MAX_FIRST_CRUSHER_GROUP_WAYPOINTS, CloserDoorCrusherFinalPoint.GetPositionX(), CloserDoorCrusherFinalPoint.GetPositionY(),
                            CloserDoorCrusherFinalPoint.GetPositionZ(), 1*IN_MILLISECONDS);
                        Start(true, true);
                        break;
                }
            }

            void WaypointReached(uint32 waypointId)
            {
                switch (waypointId)
                {
                    case 9:
                        break;
                }
            }

            void UpdateEscortAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (!me->HasAura(SPELL_FRENZY) && HealthBelowPct(30))
                {
                    DoCast(me, SPELL_FRENZY);
                    Talk(EMOTE_FRENZY);
                }

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SMASH:
                            DoCastVictim(SPELL_SMASH);
                            _events.ScheduleEvent(EVENT_SMASH, DUNGEON_MODE(urand(12, 18)*IN_MILLISECONDS, urand(8, 18)*IN_MILLISECONDS));
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_anub_ar_crusherAI(creature);
        }
};

class npc_anub_ar_champion : public CreatureScript
{
    public:
        npc_anub_ar_champion() : CreatureScript("npc_anub_ar_champion") { }

        struct npc_anub_ar_championAI : public npc_escortAI
        {
            npc_anub_ar_championAI(Creature* creature) : npc_escortAI(creature)
            {
                SetDespawnAtEnd(false);
                _instance = creature->GetInstanceScript();
                _canSpellInterrupt = false;
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                switch (me->GetEntry())
                {
                    case NPC_STATIC_CHAMPION:
                        return;
                    case NPC_PERIODIC_ATTACKING_CHAMPION:
                        if (me->GetPositionX() < 500.0f)
                        {
                            for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                                AddWaypoint(i, FirstCrusherGroupWaypoints[i].GetPositionX(), FirstCrusherGroupWaypoints[i].GetPositionY(),
                                FirstCrusherGroupWaypoints[i].GetPositionZ());
                        }
                        else
                        {
                            for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                                AddWaypoint(i, SecondCrusherGroupWaypoints[i].GetPositionX(), SecondCrusherGroupWaypoints[i].GetPositionY(),
                                SecondCrusherGroupWaypoints[i].GetPositionZ());
                        }

                        break;
                    case NPC_ANUB_CHAMPION_D_A_AND_SIDE:
                        if (me->GetPositionX() < 500.0f)
                        {
                            for (uint8 i = 0; i < MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i++)
                                AddWaypoint(i, FrontDoorAStairsWaypoints[i].GetPositionX(), FrontDoorAStairsWaypoints[i].GetPositionY(),
                                FrontDoorAStairsWaypoints[i].GetPositionZ());

                            for (uint8 i = MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS); i++)
                                AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionX(),
                                TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionY(),
                                TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionZ());
                        }
                        else
                        {
                            for (uint8 i = 0; i < MAX_SIDE_DOOR_STAIRS_WAYPOINTS; i++)
                                AddWaypoint(i, SideDoorStairsWaypoints[i].GetPositionX(), SideDoorStairsWaypoints[i].GetPositionY(),
                                SideDoorStairsWaypoints[i].GetPositionZ());

                            for (uint8 i = MAX_SIDE_DOOR_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_SIDE_DOOR_STAIRS_WAYPOINTS); i++)
                                AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionX(),
                                TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionY(),
                                TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionZ());
                        }

                        me->SetReactState(REACT_DEFENSIVE);
                        break;
                    case NPC_ANUB_CHAMPION_D_B:
                        for (uint8 i = 0; i < MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS; i++)
                            AddWaypoint(i, FrontDoorBStairsWaypoints[i].GetPositionX(), FrontDoorBStairsWaypoints[i].GetPositionY(),
                            FrontDoorBStairsWaypoints[i].GetPositionZ());

                        for (uint8 i = MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS); i++)
                            AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionX(),
                            TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionY(),
                            TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionZ());

                        me->SetReactState(REACT_DEFENSIVE);
                        break;
                }

                Start(true, true);
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case CREATURE_GROUP_CRUSHER_REINFORCEMENTS_A:
                        for (uint8 i = 0; i < MAX_SECOND_CRUSHER_GROUP_WAYPOINTS; i++)
                            AddWaypoint(i, FirstCrusherGroupWaypoints[i].GetPositionX(), FirstCrusherGroupWaypoints[i].GetPositionY(),
                            FirstCrusherGroupWaypoints[i].GetPositionZ());

                        AddWaypoint(MAX_FIRST_CRUSHER_GROUP_WAYPOINTS, DistantDoorChampionFinalPoint.GetPositionX(), DistantDoorChampionFinalPoint.GetPositionY(),
                            DistantDoorChampionFinalPoint.GetPositionZ(), 1*IN_MILLISECONDS);
                        Start(true, true);
                        break;
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                _events.ScheduleEvent(EVENT_REND, DUNGEON_MODE(urand(6, 9)*IN_MILLISECONDS, urand(4, 7)*IN_MILLISECONDS));
                _events.ScheduleEvent(EVENT_PUMMEL, DUNGEON_MODE(urand(14, 17)*IN_MILLISECONDS, urand(9, 12)*IN_MILLISECONDS));
            }

            void EnterEvadeMode()
            {
            }

            void WaypointReached(uint32 waypointId)
            {
                switch (waypointId)
                {
                    case 9:
                        if (me->GetEntry() == NPC_PERIODIC_ATTACKING_CHAMPION)
                        {
                            if (Unit* victim = me->SelectNearestPlayer(60.0f))
                                me->SetInCombatWith(victim);

                            break;
                        }
                        else

                            break;
                }
            }

            void UpdateEscortAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (_canSpellInterrupt && me->getVictim() && me->getVictim()->IsNonMeleeSpellCasted(false, false, true))
                {
                    DoCastVictim(SPELL_PUMMEL);
                    _canSpellInterrupt = false;
                }

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_REND:
                            if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, false, -(DUNGEON_MODE(SPELL_REND_N, SPELL_REND_H))))
                                DoCast(victim, SPELL_REND_N);

                            _events.ScheduleEvent(EVENT_REND, DUNGEON_MODE(urand(17, 32)*IN_MILLISECONDS, urand(15, 18)*IN_MILLISECONDS));
                            break;
                        case EVENT_PUMMEL:
                            _canSpellInterrupt = true;
                            _events.ScheduleEvent(EVENT_PUMMEL, DUNGEON_MODE(urand(14, 17)*IN_MILLISECONDS, urand(9, 12)*IN_MILLISECONDS));
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
            bool _canSpellInterrupt; // Used to control when the creature will be able to do next spell interrupt.
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_anub_ar_championAI(creature);
        }
};

class npc_anub_ar_necromancer : public CreatureScript
{
    public:
        npc_anub_ar_necromancer() : CreatureScript("npc_anub_ar_necromancer") { }

        struct npc_anub_ar_necromancerAI : public npc_escortAI
        {
            npc_anub_ar_necromancerAI(Creature* creature) : npc_escortAI(creature)
            {
                SetDespawnAtEnd(false);
                _instance = creature->GetInstanceScript();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                switch (me->GetEntry())
                {
                    case NPC_STATIC_NECROMANCER:
                        return;
                    case NPC_PERIODIC_ATTACKING_NECRO:
                        if (me->GetPositionX() < 500.0f)
                        {
                            for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                                AddWaypoint(i, FirstCrusherGroupWaypoints[i].GetPositionX(), FirstCrusherGroupWaypoints[i].GetPositionY(),
                                FirstCrusherGroupWaypoints[i].GetPositionZ());
                        }
                        else
                        {
                            for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                                AddWaypoint(i, SecondCrusherGroupWaypoints[i].GetPositionX(), SecondCrusherGroupWaypoints[i].GetPositionY(),
                                SecondCrusherGroupWaypoints[i].GetPositionZ());
                        }

                        break;
                    case NPC_ANUB_NECRO_D_A_AND_SIDE:
                        if (me->GetPositionX() < 500.0f)
                        {
                            for (uint8 i = 0; i < MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i++)
                                AddWaypoint(i, FrontDoorAStairsWaypoints[i].GetPositionX(), FrontDoorAStairsWaypoints[i].GetPositionY(),
                                FrontDoorAStairsWaypoints[i].GetPositionZ());

                            for (uint8 i = MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS); i++)
                                AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionX(),
                                TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionY(),
                                TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionZ());
                        }
                        else
                        {
                            for (uint8 i = 0; i < MAX_SIDE_DOOR_STAIRS_WAYPOINTS; i++)
                                AddWaypoint(i, SideDoorStairsWaypoints[i].GetPositionX(), SideDoorStairsWaypoints[i].GetPositionY(),
                                SideDoorStairsWaypoints[i].GetPositionZ());

                            for (uint8 i = MAX_SIDE_DOOR_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_SIDE_DOOR_STAIRS_WAYPOINTS); i++)
                                AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionX(),
                                TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionY(),
                                TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionZ());
                        }

                        me->SetReactState(REACT_DEFENSIVE);
                        break;
                    case NPC_ANUB_NECRO_D_B:
                        for (uint8 i = 0; i < MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS; i++)
                            AddWaypoint(i, FrontDoorBStairsWaypoints[i].GetPositionX(), FrontDoorBStairsWaypoints[i].GetPositionY(),
                            FrontDoorBStairsWaypoints[i].GetPositionZ());

                        for (uint8 i = MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS); i++)
                            AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionX(),
                            TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionY(),
                            TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionZ());

                        me->SetReactState(REACT_DEFENSIVE);
                        break;
                }

                Start(true, true);
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case CREATURE_GROUP_CRUSHER_REINFORCEMENTS_A:
                        for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                            AddWaypoint(i, FirstCrusherGroupWaypoints[i].GetPositionX(), FirstCrusherGroupWaypoints[i].GetPositionY(),
                            FirstCrusherGroupWaypoints[i].GetPositionZ());

                        AddWaypoint(MAX_FIRST_CRUSHER_GROUP_WAYPOINTS, DistantDoorNecroFinalPoint.GetPositionX(), DistantDoorNecroFinalPoint.GetPositionY(),
                            DistantDoorNecroFinalPoint.GetPositionZ(), 1*IN_MILLISECONDS);
                        Start(true, true);
                        break;
                    case CREATURE_GROUP_CRUSHER_REINFORCEMENTS_B:
                        for (uint8 i = 0; i < MAX_SECOND_CRUSHER_GROUP_WAYPOINTS; i++)
                            AddWaypoint(i, SecondCrusherGroupWaypoints[i].GetPositionX(), SecondCrusherGroupWaypoints[i].GetPositionY(),
                            SecondCrusherGroupWaypoints[i].GetPositionZ());

                        AddWaypoint(MAX_FIRST_CRUSHER_GROUP_WAYPOINTS, CloserDoorNecroFinalPoint.GetPositionX(), CloserDoorNecroFinalPoint.GetPositionY(),
                            CloserDoorNecroFinalPoint.GetPositionZ(), 1*IN_MILLISECONDS);
                        Start(true, true);
                        break;
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoCastVictim(SPELL_SHADOW_BOLT);
                _events.ScheduleEvent(EVENT_ANIMATED_BONNES, urand(14, 17)*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(4, 6)*IN_MILLISECONDS);
            }

            void EnterEvadeMode()
            {
            }

            void WaypointReached(uint32 waypointId)
            {
                switch (waypointId)
                {
                    case 9:
                        if (me->GetEntry() == NPC_PERIODIC_ATTACKING_NECRO)
                        {
                            if (Unit* victim = me->SelectNearestPlayer(60.0f))
                                me->SetInCombatWith(victim);

                            break;
                        }
                        else

                            break;
                }
            }

            void UpdateEscortAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHADOW_BOLT:
                            DoCastVictim(SPELL_SHADOW_BOLT);
                            _events.ScheduleEvent(EVENT_SHADOW_BOLT, urand(4, 6)*IN_MILLISECONDS);
                            break;
                        case EVENT_ANIMATED_BONNES:
                            DoCast(me, SPELL_ANIMATED_BONNES);
                            _events.ScheduleEvent(EVENT_ANIMATED_BONNES, urand(24, 28)*IN_MILLISECONDS);
                            break;
                    }
                }

                if (((me->GetPower(POWER_MANA) * 100) / me->GetMaxPower(POWER_MANA)) < 5)
                    DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_anub_ar_necromancerAI(creature);
        }
};

class npc_anub_ar_crypt_fiend : public CreatureScript
{
    public:
        npc_anub_ar_crypt_fiend() : CreatureScript("npc_anub_ar_crypt_fiend") { }

        struct npc_anub_ar_crypt_fiendAI : public npc_escortAI
        {
            npc_anub_ar_crypt_fiendAI(Creature* creature) : npc_escortAI(creature)
            {
                SetDespawnAtEnd(false);
                _instance = creature->GetInstanceScript();
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                switch (me->GetEntry())
                {
                    case NPC_STATIC_CRYPT_FIEND:
                        return;
                    case NPC_PERIODIC_ATTACKING_FIEND:
                        if (me->GetPositionX() < 500.0f)
                        {
                            for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                                AddWaypoint(i, FirstCrusherGroupWaypoints[i].GetPositionX(), FirstCrusherGroupWaypoints[i].GetPositionY(),
                                FirstCrusherGroupWaypoints[i].GetPositionZ());
                        }
                        else
                        {
                            for (uint8 i = 0; i < MAX_FIRST_CRUSHER_GROUP_WAYPOINTS; i++)
                                AddWaypoint(i, SecondCrusherGroupWaypoints[i].GetPositionX(), SecondCrusherGroupWaypoints[i].GetPositionY(),
                                SecondCrusherGroupWaypoints[i].GetPositionZ());
                        }

                        break;
                    case NPC_ANUB_FIEND_D_A_AND_SIDE:
                        if (me->GetPositionX() < 500.0f)
                        {
                            for (uint8 i = 0; i < MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i++)
                                AddWaypoint(i, FrontDoorAStairsWaypoints[i].GetPositionX(), FrontDoorAStairsWaypoints[i].GetPositionY(),
                                FrontDoorAStairsWaypoints[i].GetPositionZ());

                            for (uint8 i = MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS); i++)
                                AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionX(),
                                TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionY(),
                                TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS].GetPositionZ());
                        }
                        else
                        {
                            for (uint8 i = 0; i < MAX_SIDE_DOOR_STAIRS_WAYPOINTS; i++)
                                AddWaypoint(i, SideDoorStairsWaypoints[i].GetPositionX(), SideDoorStairsWaypoints[i].GetPositionY(),
                                SideDoorStairsWaypoints[i].GetPositionZ());

                            for (uint8 i = MAX_SIDE_DOOR_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_SIDE_DOOR_STAIRS_WAYPOINTS); i++)
                                AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionX(),
                                TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionY(),
                                TrashHeadingLairWaypoints[i - MAX_SIDE_DOOR_STAIRS_WAYPOINTS].GetPositionZ());
                        }

                        me->SetReactState(REACT_DEFENSIVE);
                        break;
                    case NPC_ANUB_FIEND_D_B:
                        for (uint8 i = 0; i < MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS; i++)
                            AddWaypoint(i, FrontDoorBStairsWaypoints[i].GetPositionX(), FrontDoorBStairsWaypoints[i].GetPositionY(),
                            FrontDoorBStairsWaypoints[i].GetPositionZ());

                        for (uint8 i = MAX_FRONT_DOOR_A_STAIRS_WAYPOINTS; i < (MAX_TRASH_HEADING_LAIR_WAYPOINTS + MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS); i++)
                            AddWaypoint(i, TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionX(),
                            TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionY(),
                            TrashHeadingLairWaypoints[i - MAX_FRONT_DOOR_B_STAIRS_WAYPOINTS].GetPositionZ());

                        me->SetReactState(REACT_DEFENSIVE);
                        break;
                }

                Start(true, true);
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case CREATURE_GROUP_CRUSHER_REINFORCEMENTS_B:
                        for (uint8 i = 0; i < MAX_SECOND_CRUSHER_GROUP_WAYPOINTS; i++)
                            AddWaypoint(i, SecondCrusherGroupWaypoints[i].GetPositionX(), SecondCrusherGroupWaypoints[i].GetPositionY(),
                            SecondCrusherGroupWaypoints[i].GetPositionZ());

                        AddWaypoint(MAX_FIRST_CRUSHER_GROUP_WAYPOINTS, CloserDoorFiendFinalPoint.GetPositionX(), CloserDoorFiendFinalPoint.GetPositionY(),
                            CloserDoorFiendFinalPoint.GetPositionZ(), 1*IN_MILLISECONDS);
                        Start(true, true);
                        break;
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                _events.ScheduleEvent(EVENT_INFECTED_WOUNDS, urand(4, 7)*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_CRUSHING_WEBS, urand(9, 12)*IN_MILLISECONDS);
            }

            void EnterEvadeMode()
            {
            }

            void WaypointReached(uint32 waypointId)
            {
                switch (waypointId)
                {
                    case 9:
                        if (me->GetEntry() == NPC_PERIODIC_ATTACKING_FIEND)
                        {
                            if (Unit* victim = me->SelectNearestPlayer(60.0f))
                                me->SetInCombatWith(victim);

                            break;
                        }
                        else

                            break;
                }
            }

            void UpdateEscortAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INFECTED_WOUNDS:
                            if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, false, -(DUNGEON_MODE(SPELL_INFECTED_WOUNDS_N, SPELL_INFECTED_WOUNDS_H))))
                                DoCast(victim, SPELL_INFECTED_WOUNDS_N);

                            _events.ScheduleEvent(EVENT_INFECTED_WOUNDS, urand(9, 12)*IN_MILLISECONDS);
                            break;
                        case EVENT_CRUSHING_WEBS:
                            if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, false))
                                DoCast(victim, SPELL_CRUSHING_WEBS);

                            _events.ScheduleEvent(EVENT_CRUSHING_WEBS, DUNGEON_MODE(urand(10, 13)*IN_MILLISECONDS, urand(13, 17)*IN_MILLISECONDS));
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_anub_ar_crypt_fiendAI(creature);
        }
};

// Spell: 53035 - Summon Anub'ar Champion Periodic
class spell_trigger_large_aoi_summon_anubar_champion_periodic : public SpellScriptLoader
{
    public:
        spell_trigger_large_aoi_summon_anubar_champion_periodic() : SpellScriptLoader("spell_trigger_large_aoi_summon_anubar_champion_periodic") { }

        class spell_trigger_large_aoi_summon_anubar_champion_periodic_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_trigger_large_aoi_summon_anubar_champion_periodic_AuraScript);

            bool Load()
            {
                _championsTickCounter = 1;
                _instance = GetCaster()->GetInstanceScript();
                return _instance != NULL;
            }

            void PeriodicTick(AuraEffect const* aurEff)
            {	
                if (_instance->GetData(DATA_KRIKTHIR_THE_GATEWATCHER_EVENT) != DONE)
                    return;

                Creature* trigger = GetCaster()->ToCreature();
                // Left front door trigger
                if (trigger->GetPositionX() < 500.0f && trigger->GetPositionZ() > 770.0f)
                {
                    if (_championsTickCounter == 1)
                        PreventDefaultAction();
                    else if (_championsTickCounter == 2)
                        PreventDefaultAction();
                    else if (_championsTickCounter == 3)
                        trigger->AI()->DoCast(53064);
                }

                // Right front door trigger
                if (trigger->GetPositionX() > 500.0f && trigger->GetPositionZ() > 770.63f)
                {
                    if (_championsTickCounter == 1)
                        trigger->AI()->DoCast(53090);
                    else if (_championsTickCounter == 2)
                        PreventDefaultAction();
                    else if (_championsTickCounter == 3)
                        PreventDefaultAction();
                }

                // Side door trigger
                if (trigger->GetPositionZ() < 740.0f)
                {
                    if (_championsTickCounter == 1)
                        PreventDefaultAction();
                    else if (_championsTickCounter == 2)
                        trigger->AI()->DoCast(53064);
                    else if (_championsTickCounter == 3)
                        PreventDefaultAction();
                }

                if (_championsTickCounter == 3 || _championsTickCounter > 3)
                    _championsTickCounter = 1;
                else
                    ++_championsTickCounter;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_trigger_large_aoi_summon_anubar_champion_periodic_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }

        private:
            InstanceScript* _instance;
            uint8 _championsTickCounter;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_trigger_large_aoi_summon_anubar_champion_periodic_AuraScript();
        }
};

// Spell: 53037 - Summon Anub'ar Crypt Fiend Periodic
class spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic : public SpellScriptLoader
{
    public:
        spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic() : SpellScriptLoader("spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic") { }

        class spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic_AuraScript);

            bool Load()
            {
                _fiendTickCounter = 1;
                _instance = GetCaster()->GetInstanceScript();
                return _instance != NULL;
            }

            void PeriodicTick(AuraEffect const* aurEff)
            {	
                if (_instance->GetData(DATA_KRIKTHIR_THE_GATEWATCHER_EVENT) != DONE)
                    return;

                // Left front door trigger
                Creature* trigger = GetCaster()->ToCreature();
                if (trigger->GetPositionX() < 500.0f && trigger->GetPositionZ() > 770.0f)
                {
                    if (_fiendTickCounter == 1)
                        trigger->AI()->DoCast(53065);
                    else if (_fiendTickCounter == 2)
                        PreventDefaultAction();
                    else if (_fiendTickCounter == 3)
                        PreventDefaultAction();
                }

                // Right front door trigger
                if (trigger->GetPositionX() > 500.0f && trigger->GetPositionZ() > 770.63f)
                {
                    if (_fiendTickCounter == 1)
                        PreventDefaultAction();
                    else if (_fiendTickCounter == 2)
                        trigger->AI()->DoCast(53091);
                    else if (_fiendTickCounter == 3)
                        PreventDefaultAction();
                }

                // Side door trigger
                if (trigger->GetPositionZ() < 740.0f)
                {
                    if (_fiendTickCounter == 1)
                        PreventDefaultAction();
                    else if (_fiendTickCounter == 2)
                        PreventDefaultAction();
                    else if (_fiendTickCounter == 3)
                        trigger->AI()->DoCast(53065);
                }

                if (_fiendTickCounter == 3 || _fiendTickCounter > 3)
                    _fiendTickCounter = 1;
                else
                    ++_fiendTickCounter;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }

        private:
            InstanceScript* _instance;
            uint8 _fiendTickCounter;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic_AuraScript();
        }
};

// Spell: 53036 - Summon Anub'ar Necromancer Periodic
class spell_trigger_large_aoi_summon_anubar_necromancer_periodic : public SpellScriptLoader
{
    public:
        spell_trigger_large_aoi_summon_anubar_necromancer_periodic() : SpellScriptLoader("spell_trigger_large_aoi_summon_anubar_necromancer_periodic") { }

        class spell_trigger_large_aoi_summon_anubar_necromancer_periodic_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_trigger_large_aoi_summon_anubar_necromancer_periodic_AuraScript);

            bool Load()
            {
                _necroTickCounter = 1;
                _instance = GetCaster()->GetInstanceScript();
                return _instance != NULL;
            }

            void PeriodicTick(AuraEffect const* aurEff)
            {	
                if (_instance->GetData(DATA_KRIKTHIR_THE_GATEWATCHER_EVENT) != DONE)
                    return;

                // Left front door trigger
                Creature* trigger = GetCaster()->ToCreature();
                if (trigger->GetPositionX() < 500.0f && trigger->GetPositionZ() > 770.0f)
                {
                    if (_necroTickCounter == 1)
                        PreventDefaultAction();
                    else if (_necroTickCounter == 2)
                        trigger->AI()->DoCast(53066);
                    else if (_necroTickCounter == 3)
                        PreventDefaultAction();
                }

                // Right front door trigger
                if (trigger->GetPositionX() > 500.0f && trigger->GetPositionZ() > 770.63f)
                {
                    if (_necroTickCounter == 1)
                        PreventDefaultAction();
                    else if (_necroTickCounter == 2)
                        PreventDefaultAction();
                    else if (_necroTickCounter == 3)
                        trigger->AI()->DoCast(53092);
                }

                // Side door trigger
                if (trigger->GetPositionZ() < 740.0f)
                {
                    if (_necroTickCounter == 1)
                        trigger->AI()->DoCast(53066);
                    else if (_necroTickCounter == 2)
                        PreventDefaultAction();
                    else if (_necroTickCounter == 3)
                        PreventDefaultAction();
                }

                if (_necroTickCounter == 3 || _necroTickCounter > 3)
                    _necroTickCounter = 1;
                else
                    ++_necroTickCounter;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_trigger_large_aoi_summon_anubar_necromancer_periodic_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }

        private:
            InstanceScript* _instance;
            uint8 _necroTickCounter;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_trigger_large_aoi_summon_anubar_necromancer_periodic_AuraScript();
        }
};

class TriggerPositionCheck
{
    public:
        TriggerPositionCheck(bool higherCheck) : _higherCheck(higherCheck) { }

        bool operator()(WorldObject* obj) const
        {
            if (obj->GetPositionZ() > 770.63f)
                return _higherCheck; // At front door

            return !_higherCheck; // At side door
        }

    private:
        bool _higherCheck;
};

// Id: 53177
class spell_hadronox_web_front_doors : public SpellScriptLoader
{
    public:
        spell_hadronox_web_front_doors() : SpellScriptLoader("spell_hadronox_web_front_doors") { }

        class spell_hadronox_web_front_doors_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hadronox_web_front_doors_SpellScript);
            
            bool Load()
            {
                _instance = GetCaster()->GetInstanceScript();
                return _instance != NULL;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // Remove side door trigger
                targets.remove_if(TriggerPositionCheck(false));
            }

            void HandleSendEvent(SpellEffIndex /*effIndex*/)
            {
                Creature* hadronox = GetCaster()->ToCreature();
                std::list<Creature*> triggersList;
                if (Creature* frontTriggerA = hadronox->GetMap()->GetCreature(_instance->GetData64(DATA_FRONT_DOOR_TRIGGER_A_GUID)))
                    triggersList.push_back(frontTriggerA);

                if (Creature* frontTriggerB = hadronox->GetMap()->GetCreature(_instance->GetData64(DATA_FRONT_DOOR_TRIGGER_B_GUID)))
                    triggersList.push_back(frontTriggerB);

                for (std::list<Creature*>::iterator itr = triggersList.begin(); itr != triggersList.end(); ++itr)
                {   
                    (*itr)->RemoveAurasDueToSpell(SPELL_SUMMON_ANUB_AR_CHAMPION_PERIODIC);
                    (*itr)->RemoveAurasDueToSpell(SPELL_SUMMON_ANUB_AR_NECROMANCER_PERIODIC);
                    (*itr)->RemoveAurasDueToSpell(SPELL_SUMMON_ANUB_AR_CRYPT_FIEND_PERIODIC);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hadronox_web_front_doors_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnEffectHit += SpellEffectFn(spell_hadronox_web_front_doors_SpellScript::HandleSendEvent, EFFECT_1, SPELL_EFFECT_SEND_EVENT);
            }

            InstanceScript* _instance;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hadronox_web_front_doors_SpellScript();
        }
};

// Id: 53177 - Web Side Doors
class spell_hadronox_web_side_door : public SpellScriptLoader
{
    public:
        spell_hadronox_web_side_door() : SpellScriptLoader("spell_hadronox_web_side_door") { }

        class spell_hadronox_web_side_door_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hadronox_web_side_door_SpellScript);

            bool Load()
            {
                _instance = GetCaster()->GetInstanceScript();
                return _instance != NULL;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // Remove front doors triggers
                targets.remove_if(TriggerPositionCheck(true));
            }

            void HandleSendEvent(SpellEffIndex /*effIndex*/)
            {
                if (Creature* trigger = GetCaster()->GetMap()->GetCreature(_instance->GetData64(DATA_SIDE_DOOR_TRIGGER_GUID)))
                {
                    trigger->RemoveAurasDueToSpell(SPELL_SUMMON_ANUB_AR_CHAMPION_PERIODIC);
                    trigger->RemoveAurasDueToSpell(SPELL_SUMMON_ANUB_AR_NECROMANCER_PERIODIC);
                    trigger->RemoveAurasDueToSpell(SPELL_SUMMON_ANUB_AR_CRYPT_FIEND_PERIODIC);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hadronox_web_side_door_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnEffectHit += SpellEffectFn(spell_hadronox_web_side_door_SpellScript::HandleSendEvent, EFFECT_1, SPELL_EFFECT_SEND_EVENT);
            }

            InstanceScript* _instance;

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hadronox_web_side_door_SpellScript();
        }
};

// Spell: 53255 - Check Reset
class spell_hadronox_event_check_reset : public SpellScriptLoader
{
    public:
        spell_hadronox_event_check_reset() : SpellScriptLoader("spell_hadronox_event_check_reset") { }

        class spell_hadronox_event_check_reset_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hadronox_event_check_reset_AuraScript);

            bool Load()
            {
                _instance = GetCaster()->GetInstanceScript();
                return _instance != NULL;
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Creature* creature = GetCaster()->ToCreature();
                if (!creature->SelectNearestPlayer(200.0f))
                    if (Creature* hadronox = creature->GetMap()->GetCreature(_instance->GetData64(DATA_HADRONOX)))
                        hadronox->AI()->DoAction(ACTION_RESET_EVENT);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Creature* creature = GetCaster()->ToCreature();
                if (!creature->SelectNearestPlayer(200.0f))
                    if (Creature* hadronox = creature->GetMap()->GetCreature(_instance->GetData64(DATA_HADRONOX)))
                        hadronox->AI()->DoAction(ACTION_RESET_EVENT);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_hadronox_event_check_reset_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_hadronox_event_check_reset_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }

            InstanceScript* _instance;
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hadronox_event_check_reset_AuraScript();
        }
};

class achievement_hadronox_denied : public AchievementCriteriaScript
{
    public:
        achievement_hadronox_denied() : AchievementCriteriaScript("achievement_hadronox_denied") { }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            return target && target->GetAI()->GetData(target->GetAI()->GetData(DATA_HADRONOX_DENIED));
        }

    private:
        uint32 _data;
};

void AddSC_boss_hadronox()
{
    new boss_hadronox();
    new npc_anub_ar_crusher();
    new npc_anub_ar_champion();
    new npc_anub_ar_necromancer();
    new npc_anub_ar_crypt_fiend();
    new spell_trigger_large_aoi_summon_anubar_champion_periodic();
    new spell_trigger_large_aoi_summon_anubar_crypt_fiend_periodic();
    new spell_trigger_large_aoi_summon_anubar_necromancer_periodic();
    new spell_hadronox_web_front_doors();
    new spell_hadronox_web_side_door();
    new spell_hadronox_event_check_reset();
    new achievement_hadronox_denied();
}
