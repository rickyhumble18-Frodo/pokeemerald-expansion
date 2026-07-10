EverGrandeCity_PokemonLeague_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, EverGrandeCity_PokemonLeague_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

EverGrandeCity_PokemonLeague_1F_OnTransition:
	setrespawn HEAL_LOCATION_EVER_GRANDE_CITY_POKEMON_LEAGUE
	setflag FLAG_LANDMARK_POKEMON_LEAGUE
	call_if_unset FLAG_ENTERED_ELITE_FOUR, EverGrandeCity_PokemonLeague_1F_EventScript_GuardsBlockDoor
	end

EverGrandeCity_PokemonLeague_1F_EventScript_GuardsBlockDoor::
	setobjectxyperm LOCALID_LEAGUE_GUARD_1, 9, 2
	setobjectxyperm LOCALID_LEAGUE_GUARD_2, 10, 2
	return

EverGrandeCity_PokemonLeague_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_LEAGUE_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

EverGrandeCity_PokemonLeague_1F_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart EverGrandeCity_PokemonLeague_1F_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
EverGrandeCity_PokemonLeague_1F_Pokemart:
	.2byte ITEM_ULTRA_BALL
	.2byte ITEM_HYPER_POTION
	.2byte ITEM_MAX_POTION
	.2byte ITEM_FULL_RESTORE
	.2byte ITEM_FULL_HEAL
	.2byte ITEM_REVIVE
	.2byte ITEM_MAX_REPEL
	pokemartlistend

@ The door guards only check for FLAG_BADGE06_GET because Winonas badge is the only one that can be skipped
@ Its assumed the player has the remaining badges
EverGrandeCity_PokemonLeague_1F_EventScript_DoorGuard::
	lockall
	goto_if_set FLAG_ENTERED_ELITE_FOUR, EverGrandeCity_PokemonLeague_1F_EventScript_GoForth
	getplayerxy VAR_TEMP_0, VAR_TEMP_1
	call_if_ge VAR_TEMP_0, 11, EverGrandeCity_PokemonLeague_1F_EventScript_PlayerMoveToFrontFromRight
	call_if_le VAR_TEMP_0, 8, EverGrandeCity_PokemonLeague_1F_EventScript_PlayerMoveToFrontFromLeft
	message EverGrandeCity_PokemonLeague_1F_Text_MustHaveAllGymBadges
	waitmessage
	delay 120
	goto_if_unset FLAG_BADGE06_GET, EverGrandeCity_PokemonLeague_1F_EventScript_NotAllBadges
	closemessage
	applymovement LOCALID_LEAGUE_GUARD_1, EverGrandeCity_PokemonLeague_1F_Movement_LeftGuardOutOfWay
	applymovement LOCALID_LEAGUE_GUARD_2, EverGrandeCity_PokemonLeague_1F_Movement_RightGuardOutOfWay
	waitmovement 0
	delay 10
	playfanfare MUS_OBTAIN_BADGE
	message EverGrandeCity_PokemonLeague_1F_Text_GoForth
	waitmessage
	waitfanfare
	closemessage
	copyobjectxytoperm LOCALID_LEAGUE_GUARD_1
	copyobjectxytoperm LOCALID_LEAGUE_GUARD_2
	setflag FLAG_ENTERED_ELITE_FOUR
	releaseall
	end

EverGrandeCity_PokemonLeague_1F_EventScript_PlayerMoveToFrontFromRight::
	applymovement LOCALID_PLAYER, EverGrandeCity_PokemonLeague_1F_Movement_MoveToFrontFromRight
	waitmovement 0
	return

EverGrandeCity_PokemonLeague_1F_EventScript_PlayerMoveToFrontFromLeft::
	applymovement LOCALID_PLAYER, EverGrandeCity_PokemonLeague_1F_Movement_MoveToFrontFromLeft
	waitmovement 0
	return

EverGrandeCity_PokemonLeague_1F_EventScript_NotAllBadges::
	playse SE_FAILURE
	msgbox EverGrandeCity_PokemonLeague_1F_Text_HaventObtainedAllBadges, MSGBOX_DEFAULT
	releaseall
	end

EverGrandeCity_PokemonLeague_1F_EventScript_GoForth::
	applymovement VAR_LAST_TALKED, Common_Movement_FacePlayer
	waitmovement 0
	msgbox EverGrandeCity_PokemonLeague_1F_Text_GoForth, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_FaceOriginalDirection
	waitmovement 0
	releaseall
	end

EverGrandeCity_PokemonLeague_1F_Movement_MoveToFrontFromRight:
	walk_down
	walk_left
	walk_in_place_faster_up
	step_end

EverGrandeCity_PokemonLeague_1F_Movement_MoveToFrontFromLeft:
	walk_down
	walk_right
	walk_in_place_faster_up
	step_end

EverGrandeCity_PokemonLeague_1F_Movement_LeftGuardOutOfWay:
	walk_left
	walk_in_place_faster_down
	step_end

EverGrandeCity_PokemonLeague_1F_Movement_RightGuardOutOfWay:
	walk_right
	walk_in_place_faster_down
	step_end

EverGrandeCity_PokemonLeague_1F_Text_MustHaveAllGymBadges:
	.string "Beyond this point, only those TRAINERS\n"
	.string "who have collected all the GYM BADGES\l"
	.string "are permitted to enter.\p"
	.string "TRAINER, let us confirm that you have\n"
	.string "all the GYM BADGES.$"

EverGrandeCity_PokemonLeague_1F_Text_HaventObtainedAllBadges:
	.string "You haven't obtained all the BADGES.\p"
	.string "If you're bound for the POKéMON\n"
	.string "LEAGUE, you must return with them.$"

EverGrandeCity_PokemonLeague_1F_Text_GoForth:
	.string "TRAINER! Believe in yourself and your\n"
	.string "POKéMON, and go forth!$"


EverGrandeCity_PokemonLeague_1F_EventScript_LoopScholar::
	lock
	faceplayer
	special BufferEliteFourLoopStats
	goto_if_eq VAR_ELITE_FOUR_LOOPS, 0, EverGrandeCity_PokemonLeague_1F_EventScript_LoopScholarNoClears
	msgbox EverGrandeCity_PokemonLeague_1F_Text_LoopScholarLoops, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PokemonLeague_1F_EventScript_LoopScholarNoClears::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_LoopScholarNoClears, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PokemonLeague_1F_Text_LoopScholarNoClears:
	.string "I study the ELITE FOUR's strength.\p"
	.string "No one has conquered the League yet.\n"
	.string "They await challengers at Lv{STR_VAR_2} to\l"
	.string "Lv{STR_VAR_3}.\p"
	.string "They say each time someone wins,\n"
	.string "the LEAGUE only grows stronger…$"

EverGrandeCity_PokemonLeague_1F_Text_LoopScholarLoops:
	.string "I study the ELITE FOUR's strength.\p"
	.string "The League has been conquered\n"
	.string "{STR_VAR_1} time(s) now.\p"
	.string "Fueled by defeat, the ELITE FOUR\n"
	.string "currently battle at Lv{STR_VAR_2} to Lv{STR_VAR_3}!\p"
	.string "Their prize money has swelled to\n"
	.string "match. There is no end to this…$"

EverGrandeCity_PokemonLeague_1F_EventScript_NatureSage::
	lock
	faceplayer
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageIntro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EverGrandeCity_PokemonLeague_1F_EventScript_SageDecline
	showmoneybox 0, 0
	checkmoney 5000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_SageNoMoney
	getpartysize
	goto_if_eq VAR_RESULT, 0, EverGrandeCity_PokemonLeague_1F_EventScript_SageCancel
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageWhichMon, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_ge VAR_0x8004, PARTY_SIZE, EverGrandeCity_PokemonLeague_1F_EventScript_SageCancel
	specialvar VAR_RESULT, ScriptGetPartyMonSpecies
	goto_if_eq VAR_RESULT, SPECIES_EGG, EverGrandeCity_PokemonLeague_1F_EventScript_SageEgg
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageWhichStatUp, MSGBOX_DEFAULT
	multichoice 0, 0, MULTI_NATURE_STAT_UP, FALSE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EverGrandeCity_PokemonLeague_1F_EventScript_SageCancel
	copyvar VAR_0x8005, VAR_RESULT
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageWhichStatDown, MSGBOX_DEFAULT
	multichoice 0, 0, MULTI_NATURE_STAT_DOWN, FALSE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, EverGrandeCity_PokemonLeague_1F_EventScript_SageCancel
	copyvar VAR_0x8006, VAR_RESULT
	special ComputeNatureFromStatChoices
	bufferpartymonnick STR_VAR_1, VAR_0x8004
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageConfirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EverGrandeCity_PokemonLeague_1F_EventScript_SageCancel
	special ComputeNatureFromStatChoices
	special SetHiddenNature
	removemoney 5000
	updatemoneybox
	playfanfare MUS_OBTAIN_ITEM
	waitfanfare
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageDone, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

EverGrandeCity_PokemonLeague_1F_EventScript_SageDecline::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageDecline, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PokemonLeague_1F_EventScript_SageNoMoney::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageNoMoney, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

EverGrandeCity_PokemonLeague_1F_EventScript_SageEgg::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageEgg, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

EverGrandeCity_PokemonLeague_1F_EventScript_SageCancel::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_SageDecline, MSGBOX_DEFAULT
	hidemoneybox
	release
	end

EverGrandeCity_PokemonLeague_1F_Text_SageIntro:
	.string "I am a sage of temperament.\p"
	.string "For ¥5,000, I can reshape a POKéMON's\n"
	.string "nature - which stats flourish and\l"
	.string "which wither.\p"
	.string "Shall we begin?$"

EverGrandeCity_PokemonLeague_1F_Text_SageWhichMon:
	.string "Which POKéMON shall I attune?$"

EverGrandeCity_PokemonLeague_1F_Text_SageWhichStatUp:
	.string "Which stat should FLOURISH?$"

EverGrandeCity_PokemonLeague_1F_Text_SageWhichStatDown:
	.string "And which stat may WITHER?\p"
	.string "Choosing the same stat leaves the\n"
	.string "temperament balanced.$"

EverGrandeCity_PokemonLeague_1F_Text_SageConfirm:
	.string "{STR_VAR_1} shall become {STR_VAR_2} natured\n"
	.string "for ¥5,000. Proceed?$"

EverGrandeCity_PokemonLeague_1F_Text_SageDone:
	.string "It is done.\p"
	.string "{STR_VAR_1}'s spirit is now {STR_VAR_2}.\n"
	.string "Its stats have already shifted.$"

EverGrandeCity_PokemonLeague_1F_Text_SageDecline:
	.string "Return should your POKéMON's spirit\n"
	.string "need refinement.$"

EverGrandeCity_PokemonLeague_1F_Text_SageNoMoney:
	.string "You lack the ¥5,000 fee.$"

EverGrandeCity_PokemonLeague_1F_Text_SageEgg:
	.string "An EGG's spirit is not yet formed.$"

EverGrandeCity_PokemonLeague_1F_EventScript_Quartermaster::
	lock
	faceplayer
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMIntro, MSGBOX_DEFAULT
	showmoneybox 0, 0
EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu::
	setvar VAR_0x8004, SCROLL_MULTI_LEAGUE_QUARTERMASTER
	special ShowScrollableMultichoice
	waitstate
	switch VAR_RESULT
	case 0, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyFullRestore
	case 1, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyMaxRevive
	case 2, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyMaxElixir
	case 3, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXAttack
	case 4, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXDefense
	case 5, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXSpAtk
	case 6, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXSpDef
	case 7, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXSpeed
	case 8, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyGuardSpec
	case 9, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyDireHit
	case 10, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyRareCandy
	case 11, EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyLuckyEgg
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMExit

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyFullRestore::
	setvar VAR_0x8005, ITEM_FULL_RESTORE
	checkmoney 9800
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_FULL_RESTORE, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 9800
	updatemoneybox
	additem ITEM_FULL_RESTORE, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyMaxRevive::
	setvar VAR_0x8005, ITEM_MAX_REVIVE
	checkmoney 24000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_MAX_REVIVE, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 24000
	updatemoneybox
	additem ITEM_MAX_REVIVE, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyMaxElixir::
	setvar VAR_0x8005, ITEM_MAX_ELIXIR
	checkmoney 12000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_MAX_ELIXIR, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 12000
	updatemoneybox
	additem ITEM_MAX_ELIXIR, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXAttack::
	setvar VAR_0x8005, ITEM_X_ATTACK
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_X_ATTACK, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_X_ATTACK, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXDefense::
	setvar VAR_0x8005, ITEM_X_DEFENSE
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_X_DEFENSE, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_X_DEFENSE, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXSpAtk::
	setvar VAR_0x8005, ITEM_X_SP_ATK
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_X_SP_ATK, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_X_SP_ATK, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXSpDef::
	setvar VAR_0x8005, ITEM_X_SP_DEF
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_X_SP_DEF, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_X_SP_DEF, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyXSpeed::
	setvar VAR_0x8005, ITEM_X_SPEED
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_X_SPEED, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_X_SPEED, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyGuardSpec::
	setvar VAR_0x8005, ITEM_GUARD_SPEC
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_GUARD_SPEC, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_GUARD_SPEC, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyDireHit::
	setvar VAR_0x8005, ITEM_DIRE_HIT
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_DIRE_HIT, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 3000
	updatemoneybox
	additem ITEM_DIRE_HIT, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyRareCandy::
	setvar VAR_0x8005, ITEM_RARE_CANDY
	checkmoney 40000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_RARE_CANDY, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 40000
	updatemoneybox
	additem ITEM_RARE_CANDY, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMBuyLuckyEgg::
	setvar VAR_0x8005, ITEM_LUCKY_EGG
	checkmoney 100000
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney
	checkitemspace ITEM_LUCKY_EGG, 1
	goto_if_eq VAR_RESULT, FALSE, EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom
	removemoney 100000
	updatemoneybox
	additem ITEM_LUCKY_EGG, 1
	playse SE_SHOP
	waitse
	bufferitemname STR_VAR_1, VAR_0x8005
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMPurchased, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMNoMoney::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMNoMoney, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMNoRoom::
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMNoRoom, MSGBOX_DEFAULT
	goto EverGrandeCity_PokemonLeague_1F_EventScript_QMMenu

EverGrandeCity_PokemonLeague_1F_EventScript_QMExit::
	hidemoneybox
	msgbox EverGrandeCity_PokemonLeague_1F_Text_QMBye, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PokemonLeague_1F_Text_QMIntro:
	.string "Supplies for the endless League!\n"
	.string "Premium stock, premium prices.$"

EverGrandeCity_PokemonLeague_1F_Text_QMPurchased:
	.string "{STR_VAR_1} - a fine choice for\n"
	.string "a League loop.$"

EverGrandeCity_PokemonLeague_1F_Text_QMNoMoney:
	.string "Your wallet's too light for that.\n"
	.string "Go squeeze the ELITE FOUR again.$"

EverGrandeCity_PokemonLeague_1F_Text_QMNoRoom:
	.string "Your BAG has no room for it.$"

EverGrandeCity_PokemonLeague_1F_Text_QMBye:
	.string "Come back before your next run!$"
