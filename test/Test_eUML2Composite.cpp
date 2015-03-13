// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// eUML2 stt grammar
#define BOOST_MPL_LIMIT_STRING_SIZE 80
#define MPLLIBS_LIMIT_STRING_SIZE BOOST_MPL_LIMIT_STRING_SIZE
#include <boost/msm/front/euml2/euml2.hpp>

#include <boost/test/unit_test.hpp>

using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

namespace
{
    // events
    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    struct cd_detected
    {
        cd_detected(std::string name, DiskTypeEnum diskType)
            : name(name),
            disc_type(diskType)
        {}

        std::string name;
        DiskTypeEnum disc_type;
    };

    struct counting_base_state
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {++entry_counter;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {++exit_counter;}
        int entry_counter=0;
        int exit_counter=0;
    };
    // submachine
    struct Playing_ : public msm::front::state_machine_def<Playing_,counting_base_state>
    {
        // the initial state of the player SM. Must be defined
        using initial_state = BOOST_MSM_EUML2_STATE("Song1",Playing_) ;
        using counting_base_state::on_entry;
        using counting_base_state::on_exit;
        // Transition table for Playing
        EUML2_STT(
            Playing_,
            EUML2_STT_CFG(),
            //     +---------------------------------------------------------------------------------------+
            EUML2_ROW("Song1 + next_song  -> Song2"),
            EUML2_ROW("Song2 + prev_song  -> Song1"),
            EUML2_ROW("Song2 + next_song  -> Song3"),
            EUML2_ROW("Song3 + prev_song  -> Song2")
            //     +---------------------------------------------------------------------------------------+
        )
    };
    // back-end
    typedef msm::back::state_machine<Playing_> Playing;

    // front-end: define the FSM structure
    struct player_ : public msm::front::state_machine_def<player_,counting_base_state>
    {
        unsigned int start_playback_counter;
        unsigned int can_close_drawer_counter;
        unsigned int test_fct_counter;

        player_():
        start_playback_counter(0),
        can_close_drawer_counter(0),
        test_fct_counter(0)
        {}

        // the initial state of the player SM. Must be defined
        using initial_state = BOOST_MSM_EUML2_STATE("Empty",player_) ;

        // Transition table for player
        EUML2_STT(
            player_,
            // list here the names for which we want to provide our own type, not the ones from eUML2
            // (for which an implementation is provided below)
            EUML2_STT_CFG(EUML2_STT_USE("cd_detected",cd_detected),EUML2_STT_USE("Playing",Playing)),
            //     +---------------------------------------------------------------------------------------+
            EUML2_ROW("Stopped + play        [dummy] / start_pback,test_fct -> Playing"),
            EUML2_ROW("Stopped + open_close              / open_drawer   -> Open"),
            EUML2_ROW("Stopped + stop                                    -> Stopped"),
            EUML2_ROW("Open    + open_close  [can_close] / close_drawer  -> Empty"),
            EUML2_ROW("Empty   + open_close              / open_drawer   -> Open"),
            EUML2_ROW("Empty   + cd_detected [good_disk && always_true] / store_cd -> Stopped"),
            EUML2_ROW("Playing + stop                    / stop_pback    -> Stopped"),
            EUML2_ROW("Playing + pause                   / pause_pback   -> Paused"),
            EUML2_ROW("Playing + open_close              / stop_and_open -> Open"),
            EUML2_ROW("Paused  + end_pause               / resume_pback  -> Playing"),
            EUML2_ROW("Paused  + stop                    / stop_pback    -> Stopped"),
            EUML2_ROW("Paused  + open_close              / stop_and_open -> Open")
            //     +---------------------------------------------------------------------------------------+
        )
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const&, FSM&,int)
        {
            BOOST_ERROR("no_transition called!");
        }
        // init counters
        template <class Event,class FSM>
        void on_entry(Event const&,FSM&)
        {
        }

    };
    // Pick a back-end
    typedef msm::back::state_machine<player_> player;
}

// transition actions (the standard way)
template<>
template <class Event,class Fsm,class SourceState,class TargetState>
void BOOST_MSM_EUML2_ACTION_IMPL("start_pback",player_)::operator()(Event const&, Fsm& fsm,SourceState& ,TargetState& )
{
    ++fsm.start_playback_counter;
}
template<>
template <class Event,class Fsm,class SourceState,class TargetState>
void BOOST_MSM_EUML2_ACTION_IMPL("store_cd",player_)::operator()(Event const&, Fsm& fsm,SourceState& ,TargetState& )
{
    fsm.process_event(BOOST_MSM_EUML2_EVENT("play",player_)());
}
template<>
template <class Event,class Fsm,class SourceState,class TargetState>
void BOOST_MSM_EUML2_ACTION_IMPL("test_fct",player_)::operator()(Event const&, Fsm& fsm,SourceState& ,TargetState& )
{
    ++fsm.test_fct_counter;
}
// guard conditions (the standard way)
template<>
template <class Event,class Fsm,class SourceState,class TargetState>
bool BOOST_MSM_EUML2_GUARD_IMPL("good_disk",player_)::operator()(Event const& evt, Fsm&,SourceState& ,TargetState& )
{
    // to test a guard condition, let's say we understand only CDs, not DVD
    if (evt.disc_type != DISK_CD)
    {
        return false;
    }
    return true;
}
template<>
template <class Event,class Fsm,class SourceState,class TargetState>
bool BOOST_MSM_EUML2_GUARD_IMPL("can_close",player_)::operator()(Event const&, Fsm& fsm,SourceState& ,TargetState& )
{
    ++fsm.can_close_drawer_counter;
    return true;
}

BOOST_AUTO_TEST_CASE( test_euml2_composite )
{
    player p;

    p.start();
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Empty",player_)&>().entry_counter == 1,"Empty entry not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("open_close",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 1,"Open should be active"); //Open
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Empty",player_)&>().exit_counter == 1,"Empty exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Open",player_)&>().entry_counter == 1,"Open entry not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("open_close",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Open",player_)&>().exit_counter == 1,"Open exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Empty",player_)&>().entry_counter == 2,"Empty entry not called correctly");
    BOOST_CHECK_MESSAGE(p.can_close_drawer_counter == 1,"guard not called correctly");

    p.process_event(
        cd_detected("louie, louie",DISK_DVD));
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 2,"Empty should be active"); //Empty
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Open",player_)&>().exit_counter == 1,"Open exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Empty",player_)&>().entry_counter == 2,"Empty entry not called correctly");

    p.process_event(
        cd_detected("louie, louie",DISK_CD));
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Empty",player_)&>().exit_counter == 2,"Empty exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Stopped",player_)&>().entry_counter == 1,"Stopped entry not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Stopped",player_)&>().exit_counter == 1,"Stopped exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().entry_counter == 1,"Playing entry not called correctly");
    BOOST_CHECK_MESSAGE(p.start_playback_counter == 1,"action not called correctly");
    BOOST_CHECK_MESSAGE(p.test_fct_counter == 1,"action not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("next_song",Playing_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().current_state()[0] == 1,"Song2 should be active");
    BOOST_CHECK_MESSAGE(
        p.get_state<Playing&>().get_state<BOOST_MSM_EUML2_STATE_IMPL("Song2",Playing_)&>().entry_counter == 1,
        "Song2 entry not called correctly");
    BOOST_CHECK_MESSAGE(
        p.get_state<Playing&>().get_state<BOOST_MSM_EUML2_STATE_IMPL("Song1",Playing_)&>().exit_counter == 1,
        "Song1 exit not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("next_song",Playing_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().current_state()[0] == 2,"Song3 should be active");
    BOOST_CHECK_MESSAGE(
        p.get_state<Playing&>().get_state<BOOST_MSM_EUML2_STATE_IMPL("Song3",Playing_)&>().entry_counter == 1,
        "Song3 entry not called correctly");
    BOOST_CHECK_MESSAGE(
        p.get_state<Playing&>().get_state<BOOST_MSM_EUML2_STATE_IMPL("Song2",Playing_)&>().exit_counter == 1,
        "Song2 exit not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("prev_song",Playing_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().current_state()[0] == 1,"Song2 should be active");
    BOOST_CHECK_MESSAGE(
        p.get_state<Playing&>().get_state<BOOST_MSM_EUML2_STATE_IMPL("Song2",Playing_)&>().entry_counter == 2,
        "Song2 entry not called correctly");
    BOOST_CHECK_MESSAGE(
        p.get_state<Playing&>().get_state<BOOST_MSM_EUML2_STATE_IMPL("Song3",Playing_)&>().exit_counter == 1,
        "Song3 exit not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("pause",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().exit_counter == 1,"Playing exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Paused",player_)&>().entry_counter == 1,"Paused entry not called correctly");

    // go back to Playing
    p.process_event(BOOST_MSM_EUML2_EVENT("end_pause",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 3,"Playing should be active"); //Playing
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Paused",player_)&>().exit_counter == 1,"Paused exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().entry_counter == 2,"Playing entry not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("pause",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 4,"Paused should be active"); //Paused
    BOOST_CHECK_MESSAGE(p.get_state<Playing&>().exit_counter == 2,"Playing exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Paused",player_)&>().entry_counter == 2,"Paused entry not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("stop",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Paused",player_)&>().exit_counter == 2,"Paused exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Stopped",player_)&>().entry_counter == 2,"Stopped entry not called correctly");

    p.process_event(BOOST_MSM_EUML2_EVENT("stop",player_)());
    BOOST_CHECK_MESSAGE(p.current_state()[0] == 0,"Stopped should be active"); //Stopped
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Stopped",player_)&>().exit_counter == 2,"Stopped exit not called correctly");
    BOOST_CHECK_MESSAGE(p.get_state<BOOST_MSM_EUML2_STATE_IMPL("Stopped",player_)&>().entry_counter == 3,"Stopped entry not called correctly");
}




