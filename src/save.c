/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  Ack 2.2 improvements copyright (C) 1994 by Stephen Dooley              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *       _/          _/_/_/     _/    _/     _/    ACK! MUD is modified    *
 *      _/_/        _/          _/  _/       _/    Merc2.0/2.1/2.2 code    *
 *     _/  _/      _/           _/_/         _/    (c)Stephen Zepp 1998    *
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3          *
 *   _/      _/      _/_/_/     _/    _/     _/                            *
 *                                                                         *
 *                        http://ackmud.nuc.net/                           *
 *                        zenithar@ackmud.nuc.net                          *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
/***************************************************************************
 * _/_/_/_/  _/    _/  _/_/_/_/ _/_/_/_/ AckFUSS is modified ACK!MUD 4.3.1 *
 * _/        _/    _/  _/       _/       copyright Matt Goff (Kline) 2008  *
 * _/_/      _/    _/  _/_/_/_/ _/_/_/_/                                   *
 * _/        _/    _/        _/       _/ Support for this code is provided *
 * _/        _/_/_/_/  _/_/_/_/ _/_/_/_/ at www.ackmud.net -- check it out!*
 ***************************************************************************/

#include "h/globals.h"

#ifndef DEC_ACT_INFO_H
#include "h/act_info.h"
#endif

#ifndef DEC_ACT_OBJ_H
#include "h/act_obj.h"
#endif

#ifndef DEC_ACT_WIZ_H
#include "h/act_wiz.h"
#endif

#ifndef DEC_BUILDTAB_H
#include "h/buildtab.h"
#endif

#ifndef DEC_COMM_H
#include "h/comm.h"
#endif

#ifndef DEC_DB_H
#include "h/db.h"
#endif

#ifndef DEC_EMAIL_H
#include "h/email.h"
#endif

#ifndef DEC_HANDLER_H
#include "h/handler.h"
#endif

#ifndef DEC_HASH_H
#include "h/hash.h"
#endif

#ifndef DEC_MAGIC_H
#include "h/magic.h"
#endif

#ifndef DEC_MONEY_H
#include "h/money.h"
#endif

#ifndef DEC_OBJ_FUN_H
#include "h/obj_fun.h"
#endif

#ifndef DEC_SAVE_H
#include "h/save.h"
#endif

#ifndef DEC_SSM_H
#include "h/ssm.h"
#endif

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST        100
static OBJ_DATA *rgObjNest[MAX_NEST];

extern int _filbuf args( ( FILE * ) );
extern bool deathmatch;  /* Deathmatch happening?        */

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
int loop_counter;
void save_char_obj( CHAR_DATA * ch )
{
    char strsave[MAX_INPUT_LENGTH];
    char tempstrsave[MAX_INPUT_LENGTH]; /* Hold temp filename here.. */
    char buf[MAX_INPUT_LENGTH];   /* hold misc stuff here.. */
    FILE *fp;
    char *nmptr, *bufptr;

    if ( deathmatch )
    {
        /*
         * Then a deathmatch is in progress...
         */
        /*
         * And therefore don't save...
         */
        return;
    }

    if ( IS_NPC( ch ) )
        return;


    if ( !IS_NPC( ch ) && ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;

   /* Stop auto-saving people during creation --Kline */
   if ( ch->desc && ch->desc->connected < CON_PLAYING && ch->desc->connected != CON_READ_MOTD )
        return;

    ch->save_time = current_time;


    /*
     * player files parsed directories by Yaz 4th Realm
     */
    if ( IS_NPC( ch ) )  /* convert spaces to . */
    {
        for ( nmptr = const_cast<char *>(ch->name.c_str()), bufptr = buf; *nmptr != 0; nmptr++ )
        {
            if ( *nmptr == ' ' )
                *( bufptr++ ) = '.';
            else
                *( bufptr++ ) = *nmptr;
        }
        *( bufptr ) = *nmptr;
    }
    else
        strcpy( buf, ch->name.c_str() );
    snprintf( strsave, MIL, "%s%s%s%s", PLAYER_DIR, initial( buf ), "/", cap_nocol( buf ) );

    /*
     * Tack on a .temp to strsave, use as tempstrsave
     */

    snprintf( tempstrsave, MIL, "%s.temp", strsave );

    if ( ( fp = file_open( tempstrsave, "w" ) ) == NULL )
    {
        monitor_chan( "Save_char_obj: file_open", MONITOR_BAD );
        perror( strsave );
    }
    else
    {
        fwrite_char( ch, fp );
        loop_counter = 0;
        if ( ch->first_carry != NULL )
            fwrite_obj( ch, ch->first_carry, fp, 0 );
        fprintf( fp, "#END\n" );
    }
    file_close(fp);

    /*
     * Now make temp file the actual pfile...
     */

    rename( tempstrsave, strsave );
    /*
     * THAT easy??
     */

    return;
}



/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA * ch, FILE * fp )
{
    AFFECT_DATA *paf;
    int cnt;
    int sn;
    int foo;
    char time_buf[MSL];
    string outstr;

    snprintf(time_buf, MSL, "%s", ctime(&current_time)); /* ctime adding a newline annoyed me --Kline */
    time_buf[strlen(time_buf)-1] = '\0';

    fprintf( fp, "#%s\n", IS_NPC( ch ) ? "MOB" : "PLAYER" );

    fprintf( fp, "Revision       %d\n", SAVE_REVISION );
    fprintf( fp, "Name           %s~\n", ch->name.c_str() );

    outstr.clear();
    fprintf( fp, "Act            " );
    for ( foo = 0; foo < MAX_BITSET; foo++ )
    {
        if ( ch->act.test(foo) )
        {
            outstr += rev_table_lookup( tab_player_act, foo );
            outstr += " ";
        }
    }
    fprintf( fp, "%sEOL\n", outstr.c_str() );

    outstr.clear();
    fprintf( fp, "Deaf           " );
    for ( foo = 0; foo < MAX_BITSET; foo++ )
    {
        if ( ch->deaf.test(foo) )
        {
            outstr += rev_chan_table_lookup( tab_channels, foo );
            outstr += " ";
        }
    }
    fprintf( fp, "%sEOL\n", outstr.c_str() );
    fprintf( fp, "ShortDescr     %s~\n", CSTR( ch->short_descr ) );
    fprintf( fp, "LongDescr      %s~\n", ch->long_descr_orig.c_str() );
    fprintf( fp, "Description    %s~\n", ch->description.c_str() );
    fprintf( fp, "Prompt         %s~\n", ch->prompt.c_str() );
    fprintf( fp, "Sex            %d\n", ch->sex );
    fprintf( fp, "LoginSex       %d\n", ch->login_sex );
    fprintf( fp, "Class          %d\n", ch->p_class );
    fprintf( fp, "Race           %d\n", ch->race );
    fprintf( fp, "Level          %d\n", ch->level );
    fprintf( fp, "Invis          %d\n", ch->invis );
    if ( !IS_NPC(ch) )
    {
        fprintf( fp, "Sentence       %d\n", ch->sentence );
    }

    fprintf( fp, "m/c            " );
    for ( cnt = 0; cnt < MAX_CLASS; cnt++ )
        fprintf( fp, "%2d ", ch->lvl[cnt] );
    fprintf( fp, "\n" );

    fprintf( fp, "Remort         " );
    for ( cnt = 0; cnt < MAX_CLASS; cnt++ )
        fprintf( fp, "%2d ", ch->lvl2[cnt] );
    fprintf( fp, "\n" );

    if ( !IS_NPC(ch) )
        fprintf( fp, "Adeptlevel     %d\n", ch->get_level("adept") );
    fprintf( fp, "Trust          %d\n", ch->trust );
    fprintf( fp, "Wizbit         %d\n", ch->wizbit );
    fprintf( fp, "Played         %d\n", ch->played + ( int )( current_time - ch->logon ) );
    fprintf( fp, "Note           %ld\n", ch->last_note );
    fprintf( fp, "Room           %d\n",
             ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
               && ch->was_in_room != NULL ) ? ch->was_in_room->vnum : ch->in_room == NULL ? get_room_index(ROOM_VNUM_LIMBO)->vnum : ch->in_room->vnum );

    fprintf( fp, "HpManaMove     %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );

    fprintf( fp, "Money          %d ", MAX_CURRENCY );
    for ( foo = 0; foo < MAX_CURRENCY; foo++ )
        fprintf( fp, "%d ", ch->money->cash_unit[foo] );
    fprintf( fp, "\n" );

    outstr.clear();
    fprintf( fp, "Monitor        " );
    for ( foo = 0; foo < MAX_BITSET; foo++ )
    {
        if ( ch->monitor.test(foo) )
        {
            outstr += rev_mon_table_lookup( tab_monitor, foo );
            outstr += " ";
        }
    }
    fprintf( fp, "%sEOL\n", outstr.c_str() );
    fprintf( fp, "BankMoney      %d ", MAX_CURRENCY );
        for ( foo = 0; foo < MAX_CURRENCY; foo++ )
            fprintf( fp, "%d ", ch->bank_money->cash_unit[foo] );
        fprintf( fp, "\n" );

    fprintf( fp, "Exp            %d\n", ch->exp );

    fprintf( fp, "AffectedBy     %d\n", ch->affected_by );
    /*
     * Bug fix from Alander
     */
    fprintf( fp, "Position       %d\n", ch->position == POS_FIGHTING ? POS_STANDING : ch->position );

    fprintf( fp, "SavingThrow    %d\n", ch->saving_throw );
    fprintf( fp, "Alignment      %d\n", ch->alignment );
    fprintf( fp, "Hitroll        %d\n", ch->hitroll );
    fprintf( fp, "Damroll        %d\n", ch->damroll );
    fprintf( fp, "Armor          %d\n", ch->armor );
    fprintf( fp, "Wimpy          %d\n", ch->wimpy );
    fprintf( fp, "Clan           %d\n", ch->clan );

    if ( IS_NPC( ch ) )
    {
        fprintf( fp, "Vnum           %d\n", ch->pIndexData->vnum );
    }
    else
    {
        fprintf( fp, "Practice       %d\n", ch->practice );
        fprintf( fp, "DeathCnt       %d\n", ch->death_cnt );
        fprintf( fp, "Order          %d %d %d %d %d\n",
                 ch->order[0], ch->order[1], ch->order[2],
                 ch->order[3], ch->order[4] );
        fprintf( fp, "Password       %s~\n", CSTR( ch->pwd ) );
        fprintf( fp, "LoadMsg        %s~\n", CSTR( ch->load_msg ) );
        fprintf( fp, "Bamfin         %s~\n", CSTR( ch->bamfin ) );
        fprintf( fp, "Bamfout        %s~\n", CSTR( ch->bamfout ) );
        fprintf( fp, "Roomenter      %s~\n", CSTR( ch->room_enter ) );
        fprintf( fp, "Roomexit       %s~\n", CSTR( ch->room_exit ) );
        fprintf( fp, "Title          %s~\n", CSTR( ch->title ) );
        /*
         * We add a '*' to preserve leading spaces... strip * on load
         */
        fprintf( fp, "Whoname        *%s~\n", CSTR( ch->who_name ) );
        fprintf( fp, "Host           ");
        for ( cnt = 0; cnt < MAX_HOSTS; cnt++ )
            fprintf(fp, "%s~", CSTR( ch->host[cnt] ) );
        fprintf( fp, "\n");
        fprintf( fp, "Whitelist      ");
        for ( cnt = 0; cnt < MAX_HOSTS; cnt++ )
            fprintf(fp, "%s~", CSTR( ch->whitelist[cnt] ) );
        fprintf( fp, "\n");
        fprintf( fp, "Failures       %d\n", ch->failures );
        fprintf( fp, "LastLogin      %s~\n", time_buf );
        fprintf( fp, "HiCol          %s~\n", CSTR( ch->col_hi ) );
        fprintf( fp, "DimCol         %s~\n", CSTR( ch->col_dim ) );
        fprintf( fp, "TermRows       %d\n", ch->term_row );
        fprintf( fp, "TermColumns    %d\n", ch->term_col );
        fprintf( fp, "Email          %s~\n", ch->email->address.c_str() );
        fprintf( fp, "EmailCode      %s~\n", ch->email->confirmation_code.c_str() );

        outstr.clear();
        fprintf( fp, "EmailFlags     " );
        for ( foo = 0; foo < MAX_BITSET; foo++ )
        {
            if ( ch->email->flags.test(foo) )
            {
                outstr += rev_table_lookup( tab_email, foo );
                outstr += " ";
            }
        }
        fprintf( fp, "%sEOL\n", outstr.c_str() );

        fprintf( fp, "EmailValid     %i\n", ch->email->verified );
        fprintf( fp, "AssistMsg      %s~\n", CSTR( ch->assist_msg ) );

        for ( cnt = 0; cnt < MAX_ALIASES; cnt++ )
        {
            fprintf( fp, "Alias_Name%d    %s~\n", cnt, CSTR( ch->alias_name[cnt] ) );
            fprintf( fp, "Alias%d         %s~\n", cnt, CSTR( ch->alias[cnt] ) );
        }

        fprintf( fp, "Colors         " );
        for ( foo = 0; foo < MAX_COLOR; foo++ )
            fprintf( fp, "%d ", ch->color[foo] );
        fprintf( fp, "\n" );

        fprintf( fp, "AttrPerm       %d %d %d %d %d\n",
                 ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con );

        fprintf( fp, "AttrMax        %d %d %d %d %d\n",
                 ch->max_str, ch->max_int, ch->max_wis, ch->max_dex, ch->max_con );

        fprintf( fp, "QuestAmount    " );
        for ( foo = 0; foo < QUEST_MAX_DATA; foo++ )
            fprintf( fp, "%d ", ch->quest_info->amount[foo] );
        fprintf( fp, "\n" );
        fprintf( fp, "QuestComplete  %d\n", ch->quest_info->quest_complete );
        fprintf( fp, "QuestHint      " );
        for ( foo = 0; foo < QUEST_MAX_DATA; foo++ )
            fprintf( fp, "%d ", ch->quest_info->quest_hint[foo] );
        fprintf( fp, "\n" );
        fprintf( fp, "QuestIsQuest   %d\n", ch->quest_info->is_questing );
        fprintf( fp, "QuestItem      " );
        for ( foo = 0; foo < QUEST_MAX_DATA; foo++ )
            fprintf( fp, "%d ", ch->quest_info->quest_item_vnum[foo] );
        fprintf( fp, "\n" );
        fprintf( fp, "QuestMob       " );
        for ( foo = 0; foo < QUEST_MAX_DATA; foo++ )
            fprintf( fp, "%d ", ch->quest_info->quest_mob_vnum[foo] );
        fprintf( fp, "\n" );
        fprintf( fp, "QuestReward    " );
        for ( foo = 0; foo < QUEST_MAX_REWARD; foo++ )
            fprintf( fp, "%d ", ch->quest_info->quest_reward[foo] );
        fprintf( fp, "\n" );
        fprintf( fp, "QuestTimeLeft  %d\n", ch->quest_info->time_left );
        fprintf( fp, "QuestType      %d\n", ch->quest_info->quest_type );
        fprintf( fp, "QuestWaitTime  %d\n", ch->quest_info->wait_time );

        fprintf( fp, "RecCrusade     %d\n", ch->records->crusade );
        fprintf( fp, "RecMdamAmt     %d\n", ch->records->mdam_amt );
        fprintf( fp, "RecMdamGsn     %d\n", ch->records->mdam_gsn );
        fprintf( fp, "RecPdamAmt     %d\n", ch->records->pdam_amt );
        fprintf( fp, "RecPdamGsn     %d\n", ch->records->pdam_gsn );
        fprintf( fp, "RecPD          %d\n", ch->records->pd );
        fprintf( fp, "RecPK          %d\n", ch->records->pk );
        fprintf( fp, "RecMD          %d\n", ch->records->md );
        fprintf( fp, "RecMK          %d\n", ch->records->mk );
        fprintf( fp, "RecMquestC     %d\n", ch->records->mquest_c );
        fprintf( fp, "RecMquestF     %d\n", ch->records->mquest_f );
        fprintf( fp, "RecQP          %d\n", ch->records->qp );
        fprintf( fp, "RecQpTot       %d\n", ch->records->qp_tot );

        fprintf( fp, "SupBloodline   %d\n", ch->super->bloodline );
        fprintf( fp, "SupEnergy      %d\n", ch->super->energy );
        fprintf( fp, "SupEnergyMax   %d\n", ch->super->energy_max );
        fprintf( fp, "SupExp         %d\n", ch->super->exp );
        fprintf( fp, "SupGeneration  %d\n", ch->super->generation );
        fprintf( fp, "SupLevel       %d\n", ch->super->level );
        fprintf( fp, "SupPracs       %d\n", ch->super->pracs );
        fprintf( fp, "SupSkillLearn  %d\n", ch->super->skills_learned );
        fprintf( fp, "SupSkillMax    %d\n", ch->super->skills_max );

        fprintf( fp, "RecoveryCode   %s~\n", ch->recovery_code.c_str() );
        fprintf( fp, "QuestPoints    %d\n", ch->quest_points );
        fprintf( fp, "RecallVnum     %d\n", ch->recall_vnum );
        fprintf( fp, "GainMana       %d\n", ch->mana_from_gain );
        fprintf( fp, "GainHp         %d\n", ch->hp_from_gain );
        fprintf( fp, "GainMove       %d\n", ch->move_from_gain );
        fprintf( fp, "RulerRank      %d\n", ch->ruler_rank );
        fprintf( fp, "Condition      %d %d %d\n", ch->condition[0], ch->condition[1], ch->condition[2] );
        fprintf( fp, "Pagelen        %d\n", ch->pagelen );

        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL && ch->learned[sn] > 0 )
            {
                fprintf( fp, "Skill          %d '%s'\n", ch->learned[sn], skill_table[sn].name );
            }
        }
        imc_savechar( ch, fp );
    }

    for ( paf = ch->first_affect; paf != NULL; paf = paf->next )
    {
        fprintf( fp, "Affect %3d %3d %3d %3d %10d\n", paf->type, paf->duration, paf->modifier, paf->location, paf->bitvector );
    }

    fprintf( fp, "End\n\n" );
    return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    short foo;
    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    loop_counter++;
    if ( loop_counter > 650 )
    {
        abort_wrapper(  );
    }
    if ( obj->next_in_carry_list != NULL )
        fwrite_obj( ch, obj->next_in_carry_list, fp, iNest );

    /*
     * Castrate storage characters.
     */

    /*
     * Also bypass no-save objects -S-
     */

    if ( ch->get_level("psuedo") + 5 < ( obj->level )
            || obj->item_type == ITEM_KEY || obj->item_type == ITEM_BEACON || IS_OBJ_STAT(obj, ITEM_EXTRA_NO_SAVE) )
        return;

    fprintf( fp, "#OBJECT\n" );
    fprintf( fp, "Nest         %d\n", iNest );
    fprintf( fp, "Name         %s~\n", obj->name );
    fprintf( fp, "ShortDescr   %s~\n", obj->short_descr );
    fprintf( fp, "LongDescr    %s~\n", obj->long_descr );
    fprintf( fp, "Durability   %d %d\n", obj->durability, obj->max_durability );
    fprintf( fp, "Vnum         %d\n", obj->pIndexData->vnum );

    fprintf( fp, "ExtraFlags   " );
    for ( short i = 0; i < MAX_BITSET; i++ )
        if ( obj->extra_flags.test(i) )
            fprintf( fp, "%d ", i );
    fprintf( fp, "EOL\n" );

    fprintf( fp, "WearFlags    " );
    for ( short i = 0; i < MAX_BITSET; i++ )
        if ( obj->wear_flags.test(i) )
            fprintf( fp, "%d ", i );
    fprintf( fp, "EOL\n" );

    fprintf( fp, "WearLoc      %d\n", obj->wear_loc );
    fprintf( fp, "Money %d ", MAX_CURRENCY );
    for ( foo = 0; foo < MAX_CURRENCY; foo++ )
        fprintf( fp, "%d ", obj->money->cash_unit[foo] );
    fprintf( fp, "\n" );
    if ( obj->obj_fun != NULL )
        fprintf( fp, "Objfun       %s~\n", rev_obj_fun_lookup( obj->obj_fun ) );

    fprintf( fp, "ClassFlags   %d\n", obj->item_apply );
    /*
     * ClassFlags still used to save fucking with pfiles
     */
    fprintf( fp, "ItemType     %d\n", obj->item_type );
    fprintf( fp, "Weight       %d\n", obj->weight );
    fprintf( fp, "Level        %d\n", obj->level );
    fprintf( fp, "Timer        %d\n", obj->timer );
    fprintf( fp, "Cost         %d\n", obj->cost );
    fprintf( fp, "Values       %d %d %d %d %d %d %d %d %d %d\n",
             obj->value[0], obj->value[1], obj->value[2], obj->value[3],
             obj->value[4], obj->value[5], obj->value[6], obj->value[7], obj->value[8], obj->value[9] );
    fprintf( fp, "Speed        %4.2f\n", obj->speed );

    switch ( obj->item_type )
    {
        case ITEM_POTION:
        case ITEM_SCROLL:
            if ( obj->value[1] > 0 )
            {
                fprintf( fp, "Spell 1      '%s'\n", skill_table[obj->value[1]].name );
            }

            if ( obj->value[2] > 0 )
            {
                fprintf( fp, "Spell 2      '%s'\n", skill_table[obj->value[2]].name );
            }

            if ( obj->value[3] > 0 )
            {
                fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name );
            }

            break;

        case ITEM_PILL:
        case ITEM_STAFF:
        case ITEM_WAND:
            if ( obj->value[3] > 0 )
            {
                fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name );
            }

            break;
    }

    for ( paf = obj->first_apply; paf != NULL; paf = paf->next )
    {
        fprintf( fp, "Affect       %d %d %d %d %d\n", paf->type, paf->duration, paf->modifier, paf->location, paf->bitvector );
    }

    for ( ed = obj->first_exdesc; ed != NULL; ed = ed->next )
    {
        fprintf( fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->first_in_carry_list != NULL )
        fwrite_obj( ch, obj->first_in_carry_list, fp, iNest + 1 );

    return;
}

/* so the stack doesn't get hosed */
void abort_wrapper( void )
{
    abort(  );
};

hash_table *hash_changed_vnums = NULL;

/*
 * Load a char and inventory into a new ch structure.
 */

int cur_revision = 0;

bool load_char_obj( DESCRIPTOR_DATA * d, const char *name, bool system_call )
{
    char strsave[MAX_INPUT_LENGTH];
    char tempstrsave[MAX_INPUT_LENGTH];
    char *bufptr;
    const char *nmptr;
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];
    FILE *fp;
    bool found;
    bool is_npc;
    int oldvnum, newvnum;

    if ( hash_changed_vnums == NULL )
    {
        /*
         * Initialise hash table for changed vnums, and read in file.
         */
        hash_changed_vnums = create_hash_table( 1024 );

        if ( ( fp = file_open( "area_changes.txt", "r" ) ) != NULL ) /* -- Alty */
        {
            while ( !feof( fp ) )
            {
                if ( str_cmp( fread_word( fp ), "Obj:" )
                        || fread_letter( fp ) != '['
                        || ( oldvnum = fread_number( fp ) ) == 0
                        || fread_letter( fp ) != ']'
                        || str_cmp( fread_word( fp ), "->" )
                        || fread_letter( fp ) != '[' || ( newvnum = fread_number( fp ) ) == 0 || fread_letter( fp ) != ']' )
                    fread_to_eol( fp );
                else
                {
                    fread_to_eol( fp );
                    add_hash_entry( hash_changed_vnums, oldvnum, ( void * )&newvnum );
                }
            }
            file_close( fp );
        }
    }

    /* load npc */
    if ( ( d == NULL ) && ( !system_call ) )
        is_npc = TRUE;
    else
        is_npc = FALSE;

    ch = new CHAR_DATA;

    if ( !is_npc )
    {
        d->character = ch;

#ifdef IMC
        imc_initchar( ch );
#endif
#ifdef I3
        i3init_char( ch );
#endif
    }

    ch->desc = d;
    ch->name = name;
    ch->prompt = DEFAULT_PROMPT;
    if ( is_npc )
        ch->npc = true;
    else
        ch->npc = false;

    found = FALSE;

    /*
     * parsed player file directories by Yaz of 4th Realm
     */
    /*
     * decompress if .gz file exists - Thx Alander
     */

    if ( is_npc )  /* convert spaces to . */
    {
        for ( nmptr = name, bufptr = buf; *nmptr != 0; nmptr++ )
        {
            if ( *nmptr == ' ' )
                *( bufptr++ ) = '.';
            else
                *( bufptr++ ) = *nmptr;
        }
        *( bufptr ) = *nmptr;
    }
    else
        strcpy( buf, name );
    snprintf( strsave, MIL, "%s%s%s%s", is_npc ? NPC_DIR : PLAYER_DIR, initial( buf ), "/", cap_nocol( buf ) );

    snprintf( tempstrsave, MIL, "%s%s", strsave, ".gz" );
    if ( ( fp = file_open( tempstrsave, "r" ) ) != NULL )
    {
        file_close( fp );
        snprintf( buf, MSL, "gzip -dfq %s", tempstrsave );
        if ( system( buf ) ) {} /* See around line 314 in email.c to explain this --Kline */
    }

    if ( ( fp = file_open( strsave, "r" ) ) != NULL )
    {
        int iNest;

        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

        found = TRUE;
        for ( ;; )
        {
            char letter;
            char *word;

            letter = fread_letter( fp );
            if ( letter == '*' )
            {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' )
            {
                monitor_chan( "Load_char_obj: # not found.", MONITOR_BAD );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "PLAYER" ) )
                fread_char( ch, fp );
            else if ( !str_cmp( word, "MOB" ) )
                fread_char( ch, fp );
            else if ( !str_cmp( word, "OBJECT" ) )
                fread_obj( ch, fp );
            else if ( !str_cmp( word, "END" ) )
                break;
            else
            {
                monitor_chan( "Load_char_obj: bad section.", MONITOR_BAD );
                break;
            }
        }
        file_close( fp );
    }

    if ( !found && is_npc )
    {
        /*
         * return memory for char back to system.
         */
        delete ch;
    }
    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )  if ( !str_cmp( word, literal ) ) { field  = value; fMatch = TRUE;  break;}
#define SKEY( literal, field, value )  if ( !str_cmp( word, literal ) ) { if (field!=NULL) free_string(field);field  = value; fMatch = TRUE;  break;}

void fread_char( CHAR_DATA * ch, FILE * fp )
{
    const char *word;
    bool fMatch;
    int cnt;

    for ( ;; )
    {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) )
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                if ( !str_cmp( word, "Act" ) )
                {
                    const char *tmp = fread_word(fp);

                    if( cur_revision >= SAVE_REVISION )
                    {
                        while ( str_cmp(tmp, "EOL") )
                        {
                            ch->act.set(table_lookup(tab_player_act,const_cast<char *>(tmp)));
                            tmp = fread_word(fp);
                        }
                    }
                    else
                    {
                        while ( str_cmp(tmp, "EOL") )
                        {
                            ch->act.set(atoi(tmp));
                            tmp = fread_word(fp);
                        }
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "AffectedBy", ch->affected_by, fread_number( fp ) );
                KEY( "Alignment", ch->alignment, fread_number( fp ) );
                KEY( "Armor", ch->armor, fread_number( fp ) );
                KEY( "AssistMsg", ch->assist_msg, fread_string( fp ) );
                for ( cnt = 0; cnt < MAX_ALIASES; cnt++ )
                {
                    KEY( "Alias_Name%d", ch->alias_name[cnt], fread_string( fp ) );
                    KEY( "Alias%d", ch->alias[cnt], fread_string( fp ) );
                }

                if ( !str_cmp( word, "Affect" ) )
                {
                    AFFECT_DATA *paf;

                    paf = new AFFECT_DATA;
                    paf->type = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    if ( paf->type != -1 )
                        LINK( paf, ch->first_saved_aff, ch->last_saved_aff, next, prev );
                    else
                        delete paf;
                    fMatch = TRUE;
                    break;
                }

                if ( !IS_NPC( ch ) )
                {
                    if ( !str_cmp( word, "AttrMax" ) )
                    {
                        ch->max_str = fread_number( fp );
                        ch->max_int = fread_number( fp );
                        ch->max_wis = fread_number( fp );
                        ch->max_dex = fread_number( fp );
                        ch->max_con = fread_number( fp );
                        fMatch = TRUE;
                        break;
                    }

                    if ( !str_cmp( word, "AttrPerm" ) )
                    {
                        ch->perm_str = fread_number( fp );
                        ch->perm_int = fread_number( fp );
                        ch->perm_wis = fread_number( fp );
                        ch->perm_dex = fread_number( fp );
                        ch->perm_con = fread_number( fp );
                        fMatch = TRUE;
                        break;
                    }
                }
                break;

            case 'B':
                KEY( "Bamfin", ch->bamfin, fread_string( fp ) );
                KEY( "Bamfout", ch->bamfout, fread_string( fp ) );
                if ( !str_cmp( word, "BankMoney" ) )
                {
                    MONEY_TYPE *transfer = new MONEY_TYPE;
                    int num_coins;

                    num_coins = fread_number( fp );
                    for ( cnt = 0; cnt < num_coins; cnt++ )
                        transfer->cash_unit[( cnt < MAX_CURRENCY ? cnt : MAX_CURRENCY - 1 )] = fread_number( fp );
                    join_money( transfer, ch->bank_money );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY( "Clan", ch->clan, fread_number( fp ) );
                KEY( "Class", ch->p_class, fread_number( fp ) );
                if ( !str_cmp( word, "Colors" ) && !IS_NPC( ch ) )
                {
                    int foo;
                    for ( foo = 0; foo < MAX_COLOR; foo++ )
                        ch->color[foo] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Condition" ) && !IS_NPC( ch ) )
                {
                    ch->condition[0] = fread_number( fp );
                    ch->condition[1] = fread_number( fp );
                    ch->condition[2] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'D':
                KEY( "Damroll", ch->damroll, fread_number( fp ) );
                if ( !str_cmp( word, "Deaf" ) )
                {
                    const char *tmp = fread_word(fp);

                    if( cur_revision >= SAVE_REVISION )
                    {
                        while ( str_cmp(tmp, "EOL") )
                        {
                            ch->deaf.set(chan_table_lookup(tab_channels,const_cast<char *>(tmp)));
                            tmp = fread_word(fp);
                        }
                    }
                    else
                    {
                        while ( str_cmp(tmp, "EOL") )
                        {
                            ch->deaf.set(atoi(tmp));
                            tmp = fread_word(fp);
                        }
                    }
                    fMatch = TRUE;
                    break;
                }

                if ( !IS_NPC(ch) )
                    KEY("DeathCnt", ch->death_cnt, fread_number(fp) );
                KEY( "Description", ch->description, fread_string( fp ) );
                KEY( "DimCol", ch->col_dim, fread_string( fp ) );
                break;

            case 'E':
                KEY( "Email", ch->email->address, fread_string( fp ) );
                KEY( "EmailCode", ch->email->confirmation_code, fread_string( fp ) );
                if ( !str_cmp( word, "EmailFlags" ) )
                {
                    const char *tmp = fread_word(fp);

                    while ( str_cmp(tmp, "EOL") )
                    {
                        ch->email->flags.set(table_lookup(tab_email,const_cast<char *>(tmp)));
                        tmp = fread_word(fp);
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "EmailValid", ch->email->verified, fread_number( fp ) );
                if ( !str_cmp( word, "End" ) )
                {
                    if ( ch->login_sex < 0 )
                        ch->login_sex = ch->sex;
                    return;
                }
                KEY( "Exp", ch->exp, fread_number( fp ) );
                break;

            case 'F':
                if ( !IS_NPC( ch ) )
                {
                    KEY( "Failures", ch->failures, fread_number( fp ) );
                }
                break;

            case 'G':
                KEY( "GainHp", ch->hp_from_gain, fread_number( fp ) );
                KEY( "GainMana", ch->mana_from_gain, fread_number( fp ) );
                KEY( "GainMove", ch->move_from_gain, fread_number( fp ) );
                break;

            case 'H':
                KEY( "Hitroll", ch->hitroll, fread_number( fp ) );
                if ( !str_cmp( word, "Host" ) )
                {
                    if ( cur_revision >= SAVE_REVISION )
                    {
                        for ( short i = 0; i < MAX_HOSTS; i++ )
                            ch->host[i] = fread_string( fp );
                    }
                    else
                        ch->host[0] = fread_string(fp);

                    fMatch = TRUE;
                    break;
                }
                KEY( "HiCol", ch->col_hi, fread_string( fp ) );
                if ( !str_cmp( word, "HpManaMove" ) )
                {
                    ch->hit = fread_number( fp );
                    ch->max_hit = fread_number( fp );
                    ch->mana = fread_number( fp );
                    ch->max_mana = fread_number( fp );
                    ch->move = fread_number( fp );
                    ch->max_move = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'I':
                KEY( "Invis", ch->invis, fread_number( fp ) );
#ifdef IMC
                if ( ( fMatch = imc_loadchar( ch, fp, word ) ) )
                    break;
#endif
#ifdef I3
                if ( ( fMatch = i3load_char( ch, fp, word ) ) )
                    break;
#endif
                break;



            case 'L':
                KEY( "LastLogin", ch->last_login, fread_string( fp ) );
                KEY( "Level", ch->level, fread_number( fp ) );
                KEY( "LoadMsg", ch->load_msg, fread_string( fp ) );
                KEY( "LoginSex", ch->login_sex, fread_number( fp ) );
                KEY( "LongDescr", ch->long_descr, fread_string( fp ) );
                break;

            case 'M':
                if ( !str_cmp( word, "Monitor" ) )
                {
                    const char *tmp = fread_word(fp);

                    if( cur_revision >= SAVE_REVISION )
                    {
                        while ( str_cmp(tmp, "EOL") )
                        {
                            ch->monitor.set(mon_table_lookup(tab_monitor,const_cast<char *>(tmp)));
                            tmp = fread_word(fp);
                        }
                    }
                    else
                    {
                        while ( str_cmp(tmp, "EOL") )
                        {
                            ch->monitor.set(atoi(tmp));
                            tmp = fread_word(fp);
                        }
                    }
                    fMatch = TRUE;
                    break;
                }
                if ( !str_cmp( word, "Money" ) )
                {
                    MONEY_TYPE *transfer = new MONEY_TYPE;
                    int num_coins;

                    num_coins = fread_number( fp );
                    for ( cnt = 0; cnt < num_coins; cnt++ )
                        transfer->cash_unit[( cnt < MAX_CURRENCY ? cnt : MAX_CURRENCY - 1 )] = fread_number( fp );
                    ch->carry_weight += money_weight( transfer );
                    join_money( transfer, ch->money );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "m/c" ) )
                {
                    switch ( cur_revision )
                    {
                        default:
                            for ( cnt = 0; cnt < MAX_CLASS; cnt++ )
                                ch->lvl[cnt] = fread_number( fp );
                            break;
                    }
                    fMatch = TRUE;
                }
                break;

            case 'N':
                if ( !str_cmp( word, "Name" ) )
                {
                    /*
                     * Name already set externally.
                     */
                    fread_to_eol( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Note", ch->last_note, fread_number( fp ) );
                break;

            case 'O':
                if ( !str_cmp( word, "Order" ) )
                {
                    for ( cnt = 0; cnt < MAX_CLASS; cnt++ )
                        ch->order[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'P':
                KEY( "Pagelen", ch->pagelen, fread_number( fp ) );
                KEY( "Password", ch->pwd, fread_string( fp ) );
                KEY( "Played", ch->played, fread_number( fp ) );
                KEY( "Position", ch->position, fread_number( fp ) );
                KEY( "Practice", ch->practice, fread_number( fp ) );
                KEY( "Prompt", ch->prompt, fread_string( fp ) );
                break;

            case 'Q':
                if ( !str_cmp( word, "QuestAmount" ) )
                {
                    for ( cnt = 0; cnt < QUEST_MAX_DATA; cnt++ )
                        ch->quest_info->amount[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "QuestComplete", ch->quest_info->quest_complete, fread_number( fp ) );
                if ( !str_cmp( word, "QuestHint" ) )
                {
                    for ( cnt = 0; cnt < QUEST_MAX_DATA; cnt++ )
                        ch->quest_info->quest_hint[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "QuestIsQuest", ch->quest_info->is_questing, fread_number( fp ) );
                if ( !str_cmp( word, "QuestItem" ) )
                {
                    for ( cnt = 0; cnt < QUEST_MAX_DATA; cnt++ )
                        ch->quest_info->quest_item_vnum[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !str_cmp( word, "QuestMob" ) )
                {
                    for ( cnt = 0; cnt < QUEST_MAX_DATA; cnt++ )
                        ch->quest_info->quest_mob_vnum[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "QuestPoints", ch->quest_points, fread_number( fp ) );
                if ( !str_cmp( word, "QuestReward" ) )
                {
                    for ( cnt = 0; cnt < QUEST_MAX_REWARD; cnt++ )
                        ch->quest_info->quest_reward[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "QuestTimeLeft", ch->quest_info->time_left, fread_number( fp ) );
                KEY( "QuestType", ch->quest_info->quest_type, fread_number( fp ) );
                KEY( "QuestWaitTime", ch->quest_info->wait_time, fread_number( fp ) );
                break;

            case 'R':
                KEY( "Race", ch->race, fread_number( fp ) );
                KEY( "RecallVnum", ch->recall_vnum, fread_number( fp ) );
                KEY( "RecCrusade", ch->records->crusade, fread_number( fp ) );
                KEY( "RecMdamAmt", ch->records->mdam_amt, fread_number( fp ) );
                KEY( "RecMdamGsn", ch->records->mdam_gsn, fread_number( fp ) );
                KEY( "RecPdamAmt", ch->records->pdam_amt, fread_number( fp ) );
                KEY( "RecPdamGsn", ch->records->pdam_gsn, fread_number( fp ) );
                KEY( "RecPD", ch->records->pd, fread_number( fp ) );
                KEY( "RecPK", ch->records->pk, fread_number( fp ) );
                KEY( "RecMD", ch->records->md, fread_number( fp ) );
                KEY( "RecMK", ch->records->mk, fread_number( fp ) );
                KEY( "RecMquestC", ch->records->mquest_c, fread_number( fp ) );
                KEY( "RecMquestF", ch->records->mquest_f, fread_number( fp ) );
                KEY( "RecoveryCode", ch->recovery_code, fread_string( fp ) );
                KEY( "RecQP", ch->records->qp, fread_number( fp ) );
                KEY( "RecQpTot", ch->records->qp_tot, fread_number( fp ) );
                KEY( "Revision", cur_revision, fread_number( fp ) );
                KEY( "Roomenter", ch->room_enter, fread_string( fp ) );
                KEY( "Roomexit", ch->room_exit, fread_string( fp ) );
                KEY( "RulerRank", ch->ruler_rank, fread_number( fp ) );

                if ( !str_cmp( word, "Remort" ) )
                {
                    for ( cnt = 0; cnt < MAX_CLASS; cnt++ )
                        ch->lvl2[cnt] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Room" ) )
                {
                    ch->in_room = get_room_index( fread_number( fp ) );
                    if ( ch->in_room == NULL )
                        ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'S':
                KEY( "SavingThrow", ch->saving_throw, fread_number( fp ) );
                KEY( "Sentence", ch->sentence, fread_number( fp ) );
                KEY( "Sex", ch->sex, fread_number( fp ) );
                if ( IS_NPC( ch ) )
                    KEY( "ShortDescr", ch->short_descr, fread_string( fp ) );

                if ( !str_cmp( word, "Skill" ) && !IS_NPC( ch ) )
                {
                    int sn;
                    int value;
                    char *skill_word;
                    value = fread_number( fp );
                    skill_word = fread_word( fp );
                    sn = skill_lookup( skill_word );
                    if ( sn < 0 )
                    {
                        snprintf( log_buf, (2 * MIL), "Loading pfile %s, unknown skill %s.", ch->name.c_str(), skill_word );
                        monitor_chan( log_buf, MONITOR_BAD );
                    }
                    else
                        ch->learned[sn] = value;
                    fMatch = TRUE;
                }

                if ( !IS_NPC( ch ) )
                {
                    KEY( "SupBloodline", ch->super->bloodline, fread_number( fp ) );
                    KEY( "SupEnergy", ch->super->energy, fread_number( fp ) );
                    KEY( "SupEnergyMax", ch->super->energy_max, fread_number( fp ) );
                    KEY( "SupExp", ch->super->exp, fread_number( fp ) );
                    KEY( "SupGeneration", ch->super->generation, fread_number( fp ) );
                    KEY( "SupLevel", ch->super->level, fread_number( fp ) );
                    KEY( "SupPracs", ch->super->pracs, fread_number( fp ) );
                    KEY( "SupSkillLearn", ch->super->skills_learned, fread_number( fp ) );
                    KEY( "SupSkillMax", ch->super->skills_max, fread_number( fp ) );
                }
                break;

            case 'T':
                KEY( "Trust", ch->trust, fread_number( fp ) );
                if ( !IS_NPC( ch ) )
                {
                    KEY( "TermRows", ch->term_row, fread_number( fp ) );
                    KEY( "TermColumns", ch->term_col, fread_number( fp ) );
                    KEY( "Title", ch->title, fread_string( fp ) );
                }
                break;

            case 'V':
                if ( !str_cmp( word, "Vnum" ) && IS_NPC( ch ) )
                {
                    ch->pIndexData = get_mob_index( fread_number( fp ) );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                if ( !str_cmp( word, "Whitelist" ) && !IS_NPC(ch) )
                {
                    if ( cur_revision >= SAVE_REVISION )
                    {
                        for ( short i = 0; i < MAX_HOSTS; i++ )
                            ch->whitelist[i] = fread_string( fp );
                    }
                    else
                        ch->whitelist[0] = fread_string(fp);

                   fMatch = TRUE;
                   break;
                }
                KEY( "Wimpy", ch->wimpy, fread_number( fp ) );
                KEY( "Wizbit", ch->wizbit, fread_number( fp ) );
                if ( !str_cmp( word, "Whoname" ) && !IS_NPC( ch ) )
                {

                    ch->who_name = fread_string( fp );
                    ch->who_name.erase( 0, 1 ); // Strip leading *
                    fMatch = TRUE;
                    break;
                }

                break;
        }

        ch->long_descr_orig = ch->long_descr;
        if ( !fMatch )
        {
            snprintf( log_buf, (2 * MIL), "Loading in pfile :%s, no match for ( %s ).", ch->name.c_str(), word );
            monitor_chan( log_buf, MONITOR_BAD );
            fread_to_eol( fp );
        }
    }


}


#define TEMP_VNUM 3090


void fread_obj( CHAR_DATA * ch, FILE * fp )
{
    OBJ_DATA *obj;
    const char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    int Temp_Obj = 0, OldVnum = 0;

    obj = new OBJ_DATA;

    fNest = FALSE;
    fVnum = TRUE;
    iNest = 0;

    for ( ;; )
    {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) )
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                if ( !str_cmp( word, "Affect" ) )
                {
                    AFFECT_DATA *paf;

                    paf = new AFFECT_DATA;
                    paf->type = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    LINK( paf, obj->first_apply, obj->last_apply, next, prev );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY( "Cost", obj->cost, fread_number( fp ) );
                KEY( "ClassFlags", obj->item_apply, fread_number( fp ) );
                break;

            case 'D':
                if ( !str_cmp( word, "Durability" ) )
                {
                    obj->durability = fread_number( fp );
                    obj->max_durability = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'E':
                if ( !str_cmp( word, "ExtraFlags" ) )
                {
                    const char *tmp = fread_word(fp);
                    while ( str_cmp(tmp, "EOL") )
                    {
                        obj->extra_flags.set(atoi(tmp));
                        tmp = fread_word(fp);
                    }
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "ExtraDescr" ) )
                {
                    EXTRA_DESCR_DATA *ed;

                    ed = new EXTRA_DESCR_DATA;
                    ed->keyword = fread_string( fp );
                    ed->description = fread_string( fp );
                    LINK( ed, obj->first_exdesc, obj->last_exdesc, next, prev );
                    fMatch = TRUE;
                }

                if ( !str_cmp( word, "End" ) )
                {
                    if ( !fNest || !fVnum )
                    {
                        AFFECT_DATA *paf;
                        EXTRA_DESCR_DATA *ed;

                        monitor_chan( "Fread_obj: incomplete object.", MONITOR_BAD );
                        while ( ( paf = obj->first_apply ) != NULL )
                        {
                            obj->first_apply = paf->next;
                            delete paf;
                        }
                        while ( ( ed = obj->first_exdesc ) != NULL )
                        {
                            obj->first_exdesc = ed->next;
                            delete ed;
                        }
                        delete obj;
                        return;
                    }
                    else
                    {
                        if ( Temp_Obj )
                        {
                            int newvnum;
                            OBJ_INDEX_DATA *pObjIndex;
                            int nMatch = 0;
                            int vnum;

                            /*
                             * One of three things:
                             * Obj Vnum was deleted
                             * Obj Vnum was moved
                             * Obj Vnum was previously deleted
                             */
                            newvnum = TEMP_VNUM;

                            if ( OldVnum != TEMP_VNUM )
                            {
                                /*
                                 * Check on move table
                                 */
                                if ( ( newvnum = *static_cast<int *>( get_hash_entry( hash_changed_vnums, OldVnum ) ) ) != 0 )
                                {
                                    obj->pIndexData = get_obj_index( newvnum );
                                    if ( obj->pIndexData == NULL )
                                    {
                                        obj->pIndexData = get_obj_index( TEMP_VNUM );
                                        newvnum = TEMP_VNUM;
                                    }
                                }
                            }

                            if ( newvnum == TEMP_VNUM )
                            {
                                /*
                                 * Scan through objects, trying to find a matching description
                                 */
                                for ( vnum = 0; nMatch < static_cast<int>(obj_index_list.size()); vnum++ )
                                {
                                    if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
                                    {
                                        nMatch++;
                                        if ( !str_cmp( obj->short_descr, pObjIndex->short_descr ) )
                                        {
                                            obj->pIndexData = pObjIndex;
                                            break;
                                        }
                                    }
                                }
                            }

                        }

                        if ( iNest == 0 || rgObjNest[iNest] == NULL )
                            obj_to_char( obj, ch );
                        else  /*
                         * if ( rgObjNest[iNest-1] == obj )
                         * obj_to_char( obj, ch );
                         * else */
                            obj_to_obj( obj, rgObjNest[iNest - 1] );
                        return;
                    }
                }
                break;

            case 'I':
                KEY( "ItemType", obj->item_type, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", obj->level, fread_number( fp ) );
                SKEY( "LongDescr", obj->long_descr, fread_string( fp ) );
                break;

            case 'M':
                if ( !str_cmp( word, "Money" ) )
                {
                    MONEY_TYPE *transfer = new MONEY_TYPE;
                    int num_coins, cnt;

                    num_coins = fread_number( fp );
                    for ( cnt = 0; cnt < num_coins; cnt++ )
                        transfer->cash_unit[( cnt < MAX_CURRENCY ? cnt : MAX_CURRENCY - 1 )] = fread_number( fp );
                    join_money( transfer, obj->money );
                    fMatch = TRUE;
                    break;
                }
            case 'N':
                SKEY( "Name", obj->name, fread_string( fp ) );

                if ( !str_cmp( word, "Nest" ) )
                {
                    iNest = fread_number( fp );
                    if ( iNest < 0 || iNest >= MAX_NEST )
                    {
                        monitor_chan( "Fread_obj: bad nest.", MONITOR_BAD );
                    }
                    else
                    {
                        rgObjNest[iNest] = obj;
                        fNest = TRUE;
                    }
                    fMatch = TRUE;
                }
                break;

            case 'O':
                /*     KEY( "Objfun", obj->obj_fun,  obj_fun_lookup( fread_string( fp ) ) );  */
                if ( !str_cmp( word, "Objfun" ) )
                {
                    char *dumpme;
                    dumpme = fread_string( fp );
                    obj->obj_fun = obj_fun_lookup( dumpme );
                    free_string( dumpme );
                    fMatch = TRUE;
                }
                break;

            case 'S':
                SKEY( "ShortDescr", obj->short_descr, fread_string( fp ) );
                KEY( "Speed", obj->speed, fread_float( fp ) );

                if ( !str_cmp( word, "Spell" ) )
                {
                    int iValue;
                    int sn;

                    iValue = fread_number( fp );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( iValue < 0 || iValue > 3 )
                    {
                        monitor_chan( "Fread_obj: bad iValue ", MONITOR_BAD );
                    }
                    else if ( sn < 0 )
                    {
                        monitor_chan( "Fread_obj: unknown skill.", MONITOR_BAD );
                    }
                    else
                    {
                        obj->value[iValue] = sn;
                    }
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'T':
                KEY( "Timer", obj->timer, fread_number( fp ) );
                break;

            case 'V':
                if ( !str_cmp( word, "Values" ) )
                {
                    int looper;

                    for ( looper = 0; looper < MAX_OBJ_VALUE; obj->value[looper] = fread_number( fp ), looper++ );

                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Vnum" ) )
                {
                    int vnum;

                    vnum = fread_number( fp );


                    if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL || vnum == TEMP_VNUM )
                    {
                        /*
                         * Set flag saying that object is temporary
                         */
                        Temp_Obj = 1;
                        OldVnum = vnum;
                        vnum = TEMP_VNUM;
                        obj->pIndexData = get_obj_index( vnum );
                    }
                    /*
                     * bug( "Fread_obj: bad vnum %d.", vnum ); This killed it.
                     */
                    else
                        fVnum = TRUE;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY( "WearLoc", obj->wear_loc, fread_number( fp ) );
                if ( !str_cmp( word, "WearFlags" ) )
                {
                    const char *tmp = fread_word(fp);
                    while ( str_cmp(tmp, "EOL") )
                    {
                        obj->wear_flags.set(atoi(tmp));
                        tmp = fread_word(fp);
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "Weight", obj->weight, fread_number( fp ) );
                break;

        }

        if ( !fMatch )
        {
            monitor_chan( "Fread_obj: no match.", MONITOR_BAD );
            fread_to_eol( fp );
        }
    }
}
void fread_corpse( FILE * fp )
{
    OBJ_DATA *obj;
    const char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    int Temp_Obj = 0, OldVnum = 0;
    int this_room_vnum;

    obj = new OBJ_DATA;
    obj_list.remove(obj); /* Don't want this tracked on two lists; ctor adds automatically --Kline */
    corpse_list.push_back(obj);

    fNest = FALSE;
    fVnum = TRUE;
    iNest = 0;
    this_room_vnum = 0;

    for ( ;; )
    {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) )
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                if ( !str_cmp( word, "Affect" ) )
                {
                    AFFECT_DATA *paf;

                    paf = new AFFECT_DATA;
                    paf->type = fread_number( fp );
                    paf->duration = fread_number( fp );
                    paf->modifier = fread_number( fp );
                    paf->location = fread_number( fp );
                    paf->bitvector = fread_number( fp );
                    LINK( paf, obj->first_apply, obj->last_apply, next, prev );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY( "Cost", obj->cost, fread_number( fp ) );
                KEY( "ClassFlags", obj->item_apply, fread_number( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "ExtraFlags" ) )
                {
                    const char *tmp = fread_word(fp);
                    while ( str_cmp(tmp, "EOL") )
                    {
                        obj->extra_flags.set(atoi(tmp));
                        tmp = fread_word(fp);
                    }
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "ExtraDescr" ) )
                {
                    EXTRA_DESCR_DATA *ed;

                    ed = new EXTRA_DESCR_DATA;
                    ed->keyword = fread_string( fp );
                    ed->description = fread_string( fp );
                    LINK( ed, obj->first_exdesc, obj->last_exdesc, next, prev );
                    fMatch = TRUE;
                }

                if ( !str_cmp( word, "End" ) )
                {
                    if ( !fNest || !fVnum )
                    {
                        AFFECT_DATA *paf;
                        EXTRA_DESCR_DATA *ed;

                        monitor_chan( "Fread_obj: incomplete object.", MONITOR_BAD );
                        while ( ( paf = obj->first_apply ) != NULL )
                        {
                            obj->first_apply = paf->next;
                            delete paf;
                        }
                        while ( ( ed = obj->first_exdesc ) != NULL )
                        {
                            obj->first_exdesc = ed->next;
                            delete ed;
                        }
                        corpse_list.remove(obj);
                        delete obj;
                        return;
                    }
                    else
                    {
                        if ( Temp_Obj )
                        {
                            int newvnum;
                            OBJ_INDEX_DATA *pObjIndex;
                            int nMatch = 0;
                            int vnum;

                            /*
                             * One of three things:
                             * Obj Vnum was deleted
                             * Obj Vnum was moved
                             * Obj Vnum was previously deleted
                             */
                            newvnum = TEMP_VNUM;

                            if ( OldVnum != TEMP_VNUM )
                            {
                                /*
                                 * Check on move table
                                 */
                                if ( ( newvnum = *static_cast<int *>( get_hash_entry( hash_changed_vnums, OldVnum ) ) ) != 0 )
                                {
                                    obj->pIndexData = get_obj_index( newvnum );
                                    if ( obj->pIndexData == NULL )
                                    {
                                        obj->pIndexData = get_obj_index( TEMP_VNUM );
                                        newvnum = TEMP_VNUM;
                                    }
                                }
                            }

                            if ( newvnum == TEMP_VNUM )
                            {
                                /*
                                 * Scan through objects, trying to find a matching description
                                 */
                                for ( vnum = 0; nMatch < static_cast<int>(obj_index_list.size()); vnum++ )
                                {
                                    if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
                                    {
                                        nMatch++;
                                        if ( !str_cmp( obj->short_descr, pObjIndex->short_descr ) )
                                        {
                                            obj->pIndexData = pObjIndex;
                                            break;
                                        }
                                    }
                                }
                            }

                        }

                        if ( iNest == 0 || rgObjNest[iNest] == NULL )
                            obj_to_room( obj, get_room_index( this_room_vnum ) );
                        else
                            obj_to_obj( obj, rgObjNest[iNest - 1] );
                        return;
                    }
                }
                break;

            case 'I':
                KEY( "ItemType", obj->item_type, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", obj->level, fread_number( fp ) );
                SKEY( "LongDescr", obj->long_descr, fread_string( fp ) );
                break;

            case 'M':
                if ( !str_cmp( word, "Money" ) )
                {
                    MONEY_TYPE *transfer = new MONEY_TYPE;
                    int num_coins, cnt;

                    num_coins = fread_number( fp );
                    for ( cnt = 0; cnt < num_coins; cnt++ )
                        transfer->cash_unit[( cnt < MAX_CURRENCY ? cnt : MAX_CURRENCY - 1 )] = fread_number( fp );
                    join_money( transfer, obj->money );
                    fMatch = TRUE;
                    break;
                }
            case 'N':
                KEY( "Name", obj->name, fread_string( fp ) );

                if ( !str_cmp( word, "Nest" ) )
                {
                    iNest = fread_number( fp );
                    if ( iNest < 0 || iNest >= MAX_NEST )
                    {
                        monitor_chan( "Fread_obj: bad nest.", MONITOR_BAD );
                    }
                    else
                    {
                        rgObjNest[iNest] = obj;
                        fNest = TRUE;
                    }
                    fMatch = TRUE;
                }
                break;

            case 'O':
                /*     KEY( "Objfun", obj->obj_fun,  obj_fun_lookup( fread_string( fp ) ) );  */
                if ( !str_cmp( word, "Objfun" ) )
                {
                    char *dumpme;
                    dumpme = fread_string( fp );
                    obj->obj_fun = obj_fun_lookup( dumpme );
                    free_string( dumpme );
                    fMatch = TRUE;
                }
                break;

            case 'S':
                SKEY( "ShortDescr", obj->short_descr, fread_string( fp ) );
                KEY( "Speed", obj->speed, fread_float( fp ) );

                if ( !str_cmp( word, "Spell" ) )
                {
                    int iValue;
                    int sn;

                    iValue = fread_number( fp );
                    sn = skill_lookup( fread_word( fp ) );
                    if ( iValue < 0 || iValue > 3 )
                    {
                        monitor_chan( "Fread_obj: bad iValue ", MONITOR_BAD );
                    }
                    else if ( sn < 0 )
                    {
                        monitor_chan( "Fread_obj: unknown skill.", MONITOR_BAD );
                    }
                    else
                    {
                        obj->value[iValue] = sn;
                    }
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'T':
                KEY( "Timer", obj->timer, fread_number( fp ) );
                break;

            case 'V':
                if ( !str_cmp( word, "Values" ) )
                {
                    obj->value[0] = fread_number( fp );
                    obj->value[1] = fread_number( fp );
                    obj->value[2] = fread_number( fp );
                    obj->value[3] = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "Vnum" ) )
                {
                    int vnum;

                    vnum = fread_number( fp );


                    if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL || vnum == TEMP_VNUM )
                    {
                        /*
                         * Set flag saying that object is temporary
                         */
                        Temp_Obj = 1;
                        OldVnum = vnum;
                        vnum = TEMP_VNUM;
                        obj->pIndexData = get_obj_index( vnum );
                    }
                    /*
                     * bug( "Fread_obj: bad vnum %d.", vnum ); This killed it.
                     */
                    else
                        fVnum = TRUE;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                if ( !str_cmp( word, "WearFlags" ) )
                {
                    const char *tmp = fread_word(fp);
                    while ( str_cmp(tmp, "EOL") )
                    {
                        obj->wear_flags.set(atoi(tmp));
                        tmp = fread_word(fp);
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY( "WearLoc", obj->wear_loc, fread_number( fp ) );
                KEY( "Weight", obj->weight, fread_number( fp ) );
                KEY( "WhereVnum", this_room_vnum, fread_number( fp ) );
                break;

        }

        if ( !fMatch )
        {
            monitor_chan( "Fread_obj: no match.", MONITOR_BAD );
            fread_to_eol( fp );
        }
    }
}


void fwrite_corpse( OBJ_DATA * obj, FILE * fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    int where_vnum = ROOM_VNUM_LIMBO;
    short foo;
    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->next_in_carry_list != NULL )
        fwrite_corpse( obj->next_in_carry_list, fp, iNest );

    if ( obj->in_room != NULL )
        where_vnum = obj->in_room->vnum;

    if ( obj->in_room == NULL && obj->in_obj == NULL )
        obj->in_room = get_room_index( ROOM_VNUM_LIMBO );


    fprintf( fp, "#OBJECT\n" );
    fprintf( fp, "WhereVnum    %d\n", where_vnum );

    fprintf( fp, "Nest         %d\n", iNest );
    fprintf( fp, "Name         %s~\n", obj->name );
    fprintf( fp, "ShortDescr   %s~\n", obj->short_descr );
    fprintf( fp, "LongDescr    %s~\n", obj->long_descr );
    fprintf( fp, "Vnum         %d\n", obj->pIndexData->vnum );

    fprintf( fp, "ExtraFlags   " );
    for ( short i = 0; i < MAX_BITSET; i++ )
        if ( obj->extra_flags.test(i) )
            fprintf( fp, "%d ", i );
    fprintf( fp, "EOL\n" );

    fprintf( fp, "WearFlags    " );
    for ( short i = 0; i < MAX_BITSET; i++ )
        if ( obj->wear_flags.test(i) )
            fprintf( fp, "%d ", i );
    fprintf( fp, "EOL\n" );

    fprintf( fp, "WearLoc      %d\n", obj->wear_loc );
    fprintf( fp, "Money %d ", MAX_CURRENCY );
    for ( foo = 0; foo < MAX_CURRENCY; foo++ )
        fprintf( fp, "%d ", obj->money->cash_unit[foo] );
    fprintf( fp, "\n" );
    if ( obj->obj_fun != NULL )
        fprintf( fp, "Objfun       %s~\n", rev_obj_fun_lookup( obj->obj_fun ) );

    fprintf( fp, "ClassFlags   %d\n", obj->item_apply );
    /*
     * ClassFlags still used to save fucking with pfiles
     */
    fprintf( fp, "ItemType     %d\n", obj->item_type );
    fprintf( fp, "Weight       %d\n", obj->weight );
    fprintf( fp, "Level        %d\n", obj->level );
    fprintf( fp, "Timer        %d\n", obj->timer );
    fprintf( fp, "Cost         %d\n", obj->cost );
    fprintf( fp, "Values       %d %d %d %d\n", obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
    fprintf( fp, "Speed        %4.2f\n", obj->speed);

    switch ( obj->item_type )
    {
        case ITEM_POTION:
        case ITEM_SCROLL:
            if ( obj->value[1] > 0 )
            {
                fprintf( fp, "Spell 1      '%s'\n", skill_table[obj->value[1]].name );
            }

            if ( obj->value[2] > 0 )
            {
                fprintf( fp, "Spell 2      '%s'\n", skill_table[obj->value[2]].name );
            }

            if ( obj->value[3] > 0 )
            {
                fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name );
            }

            break;

        case ITEM_PILL:
        case ITEM_STAFF:
        case ITEM_WAND:
            if ( obj->value[3] > 0 )
            {
                fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name );
            }

            break;
    }

    for ( paf = obj->first_apply; paf != NULL; paf = paf->next )
    {
        fprintf( fp, "Affect       %d %d %d %d %d\n", paf->type, paf->duration, paf->modifier, paf->location, paf->bitvector );
    }

    for ( ed = obj->first_exdesc; ed != NULL; ed = ed->next )
    {
        fprintf( fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->first_in_carry_list != NULL )
        fwrite_corpse( obj->first_in_carry_list, fp, iNest + 1 );

    return;
}




void save_corpses(  )
{

    FILE *fp;
    char corpse_file_name[MAX_STRING_LENGTH];
    OBJ_DATA *this_corpse;
    list<OBJ_DATA *>::iterator li;

    snprintf( corpse_file_name, MSL, "%s", CORPSE_FILE );

    if ( ( fp = file_open( corpse_file_name, "w" ) ) == NULL )
    {
        bug( "Save Corpses: file_open", 0 );
        perror( "failed open of corpse_file in save_corpses" );
    }
    else
    {
        for ( li = corpse_list.begin(); li != corpse_list.end(); li++ )
        {
            this_corpse = *li;
            fwrite_corpse( this_corpse, fp, 0 );
        }
        fprintf( fp, "#END\n\n" );

        file_close(fp);
    }
    return;

}

void save_marks( void )
{
    FILE *fp;
    MARK_DATA *mk = NULL;
    ROOM_INDEX_DATA *room = NULL;
    list<ROOM_INDEX_DATA *>::iterator ri;
    list<MARK_DATA *>::iterator mi;

    if ( booting_up )
        return;

    if ( (fp = file_open(MARKS_FILE, "w")) == NULL )
    {
        file_close(fp);
        log_f("Failed to save marks.");
        return;
    }

    for ( ri = room_index_list.begin(); ri != room_index_list.end(); ri++ )
    {
        room = *ri;

        if ( !room->mark_list.empty() )
        {
            for ( mi = room->mark_list.begin(); mi != room->mark_list.end(); mi++ )
            {
                mk = *mi;

                fprintf( fp, "Room     %d\n", mk->room_vnum ); /* must be first for sanity checks --Kline */
                fprintf( fp, "Author   %s~\n", mk->author );
                fprintf( fp, "Duration %d\n", mk->duration );
                fprintf( fp, "Message  %s~\n", mk->message );
                fprintf( fp, "Type     %d\n", mk->type );
            }
        }
    }

    file_close(fp);
    return;
}

void save_bans(  )
{

    FILE *fp;
    char ban_file_name[MAX_STRING_LENGTH];
    BAN_DATA *pban;
    list<BAN_DATA *>::iterator li;

    snprintf( ban_file_name, MSL, "%s", BANS_FILE );

    if ( ( fp = file_open( ban_file_name, "w" ) ) == NULL )
    {
        bug( "Save ban list: file_open", 0 );
        perror( "failed open of bans.lst in save_ban" );
    }
    else
    {
        for ( li = ban_list.begin(); li != ban_list.end(); li++ )
        {
            pban = *li;
            fprintf( fp, "#BAN~\n" );
            fprintf( fp, "%d\n", ( pban->newbie ? 1 : 0 ) );
            fprintf( fp, "%s~\n", pban->name );
            fprintf( fp, "%s~\n", pban->banned_by );
        }
        fprintf( fp, "#END~\n\n" );
    }


    file_close(fp);
    return;

}
