# Basic stuff for 3 kingdoms
#
# This is specific to the Three Kingdoms mud at 3k.org:5000
# If you want to use this, place this in your ~/.dirt/ directory and 
# create a MUD definition in the config file like this:
#     MUD ThreeKingdoms {
#         Host 3k.org 5000
#     }

package ThreeKingdoms;  # Creates my namespace, ThreeKingdoms
# Note that to use Dirt stuff you must prefix it by main::
use IO::Handle;
use POSIX;

# GLOBALs used by this module
use vars qw(@room_exits @unusual_exits $enemy $havephantasm $haveelemental $summons $slowed);
@room_exits = ();                     # list of the exits to the current room.
@unusual_exits = ();

# Note a REALLY GOOD IDEA would be to go into the 3k config office (up from
# Cancer's shop) and change your prompt to be something more obvious like:
#     THISISAFUCKINGPROMPT>
# and also change the while(s/^> //g) below accordingly.  Theoretically matching
# against /^> / could get you into trouble.

# This replaces the standard prompt hook.
sub prompt_grab {
    while(s/^> //g) { # In case multiple prompts on one line.  3k's prompts are so stupid.
        &main::run("/hook -T PROMPT -r '> '");
    }
    return 1;
}
# High priority to supercede all other hooks/triggers.
&main::run("/hook -F -a -p 2147483646 -T OUTPUT -t'^(> )' prompt_grab = /run -Lperl ThreeKingdoms::prompt_grab");

############################# ROOMS ###########################################
my(@dirs) = ("n", "e", "s", "w", "ne", "nw", "se", "sw", "u", "d", "vortex",
  "enter", "cave", "cavern", "out", "portal", "gate", "in", "mist", "left",
  "right", "house", "tree", "stairs", "ladder", "port", "stern",
  "forward", "hole", "alley", "leave", "tear", "guild", "chaos", "pub",
  "fantasy", "science", "newbie", "login", "shop", "bank", "smithy", "gypsy",
  "arundin", "office", "jump", "magic", "sci", "sinkhole", "door", "path",
  "flames", "omp", "gap", "arena", "equip", "exit", "forest", "tower", "archway",
  "arch", "trail", "entrance", "return", "ascend", "descend", "disembark",
  "vault", "shuttle", "puddle", "complex", "void", "light"); 
my($roomnamecolor)     = &main::str_to_color("bold_White_Blue");
my($roomexitscolor)    = &main::str_to_color("Cyan_Blue");
my($dirsstring)        = join("|", @dirs);
my($bold_Yellow_Black) = &main::str_to_color("bold_Yellow_Black");
my($White_Black)       = &main::str_to_color("White_Black");
my($colorize_brief_rooms) = 1;
my($neednextline)      = 0;
sub grab_room {
#  if(/^${bold_Yellow_Black}    There (?:is|are) [a-z]+ obvious exits?: ((?:[a-z]+|, )+)${White_Black}$/) {
  if(/^ +There (?:is|are) [a-z]+ obvious exits?: ((?:[a-z]+|, )+)(,?)/) {
    @room_exits = split("\, ", $1);
    if($2) {
      $neednextline = 1;
    } else {
      &main::run("/hook -T ROOM -r '" . &main::backslashify($_, "'") . "'");
    }
    return 1;
  } elsif (/^(.* )(\((([a-z_0-9]+|,)+)\)).?$/) {
    if(/\d+ coins \(\w+\)$/) { return 0; }
    my(@crap_exits) = grep(/^($dirsstring)[0-9]*$/, split(",", $3));
    if($#crap_exits >= 0) { @room_exits = split(",", $3); }
    else { return 0; }
    @unusual_exits = grep(!/^(n|e|s|w|ne|se|sw|nw|u|d)$/, @room_exits);
    &main::run("/hook -T ROOM -r '" . &main::backslashify($_, "'") . "'");
    if($colorize_brief_rooms) {
      $_ = "$main::CSave$roomnamecolor$1$roomexitscolor$2$main::CRestore";
    }
    return 1;
  } elsif ($neednextline) {
    if(/^          +((?:[a-z]+|, )+)(,?)/) {
      push @room_exits, split("\, ", $1);
      if($2) {
        $neednextline = 1;
      } else {
        $neednextline = 0;
        &main::run("/hook -T ROOM -r '" . &main::backslashify($_, "'") . "'");
        return 1;
      }
      return 1;
    }
  }
  return 0;
}
&main::run("/hook -N ROOM");
&main::run("/hook -F -p 100 -T OUTPUT grab_room = /run -Lperl ThreeKingdoms::grab_room");
#&main::hook_add("output", "ThreeKingdoms::grab_room", \&grab_room);
# Create a hook for rooms.  If you want your function to be called when you see a 
# room description, add this: &ThreeKingdoms::room_add(\&your_func_here);
#&main::create_standard_hooks(1, "room");  
&main::save("ThreeKingdoms::colorize_brief_rooms", \$colorize_brief_rooms);

############################# PLAYERS ###########################################
# This highlights players in the room
my(@alignments) = ("good", "very good", "peeronic", "mean", "evil", "satanic", 
  "heavenly", "neutral", "moral", "saintly", "tensoric", "malicious", "immoral", 
  "pure", "demonic", "nice", "corrupt", "righteous", "sinister");
my($playercolor)       = &main::str_to_color("bold_White_Magenta");
my($alignmentcolor)    = &main::str_to_color("Cyan_Magenta");
my($alignmentstring)   = join("|", @alignments);
my($colorize_players)  = 1;
sub grab_players {
  if(/^(.*)(\((?:$alignmentstring)\)\.?)$/) {
    &main::run("/hook -T PLAYER -r '" . &main::backslashify($_, "'") . "'");
    if($colorize_players) {
      $_ = "$main::CSave$playercolor$1$alignmentcolor$2$main::CRestore";
    }
    return 1;
  }
  return 0;
}
&main::run("/hook -N PLAYER");
&main::run("/hook -F -T OUTPUT -p 100 grab_players = /run -Lperl ThreeKingdoms::grab_players");
#&main::hook_add("output", "ThreeKingdoms::grab_players", \&grab_players);
# Create a hook for players.  If you want your function to be called when you see
# another player, add this: &ThreeKingdoms::player_add(\&your_func_here);
#&main::create_standard_hooks(1, "player");
&main::save("ThreeKingdoms::colorize_players", \$colorize_players);


# Sample module for snarfing a prompt and displaying it in a window

# Usage: gauge(count, min,max, cur)
$emptyColor = &main::str_to_color('white_red'); # note it's the background color here that's important
$filledColor = &main::str_to_color('white_green');
$windowColor = &main::str_to_color('white_blue');
sub gauge ($$$$) {
    my ($count, $min, $max, $cur, $n) = @_;
    return if($max == $min); 
    $n = &POSIX::floor((($cur - $min) / ($max-$min)) * $count);
    $n = 0 if $n < 0; # max < min or cur < min
    $n = $count if $n > $count;
    my($bar) = ' ' x $n;
    my($emptybar) = ' ' x ($count-$n);
    return $filledColor . (' ' x $n) . $emptyColor . (' ' x ($count-$n)) . $windowColor;
}   

my($nsummons)=0;
my($elementalcounter)=0;
my($haveelemental) = 0;
my($havephantasm) = 0;
my($incombat) = 0;
my($lastcnc)=0;
my($summons) = "";
my($cnc) = 0;
my($sps) = 0;
my($mgup) = 0;
my($linectr) = 0; # lines since the hpbar was seen.
# This assumes a prompt that looks like this:
# minHp/maxHp minMana/maxMana minMove/maxMove
my($breedhp) = qr/^HP: ([0-9]+)\/([0-9]+) SP: ([0-9]+)\/([0-9]+)  Psi: ([0-9]+)\/([0-9]+)/;#  Focus: ([0-9]+)%  E: ([A-Z][a-z]+)/) {
my($magehp) = qr/^ HP: ([0-9]+)\/([0-9]+) SP: ([0-9]+)\/([0-9]+)S?\/([0-9]+)%\/([0-9]+)% Sat: ([0-9]+)% Cnc: ([0-9]+)% Gols:([0-9]+)\/([0-9]+)% G2N:([0-9]+)%( A)?( S(?!S))?( SS)?( mg)?( PE)?( PG)?(?: GS)?( Mon\(...\):(\w\w))?/;
my($fremenhp) = qr/^HP: ([0-9]+)\/([0-9]+) SP: ([0-9]+)\/([0-9]+) W: ([0-9]+)\/([0-9]+)\(([0-9]+)\) L: ([0-9]+)% P: ([0-9]+)\/([0-9]+)% T: [a-z]+ G: ([0-9]+)/;
sub check_hpbar {
    $linectr++;
    if(/$magehp/) {
        $linectr = 0;
        &main::run("/clear hpbar");
        &main::run("/echo -W hpbar \"HP:  " . sprintf("(%3d/%3d)", $1, $2) . " " 
            . &gauge(10, 1, $2, $1) . "\"");
        &main::run("/echo -W hpbar \"SP:  " . sprintf("(%3d/%3d)", $3, $4) . " " 
            . &gauge(10, 1, $4, $3) . "\"");
        $sps = $3;
        &main::run("/echo -W hpbar \"Sat: " . sprintf("(%3d/%3d)", $7, 100) . " " 
            . &gauge(10,1, 100, 100-$7) . "\"");
        &main::run("/echo -W hpbar \"Cnc: " . sprintf("(%3d/%3d)", $8, 100) . " " 
            . &gauge(10,1, 100, 100-$8) . "\"");
        $cnc = $8;
        if(defined $18) {
            $incombat=1;
            my($percent);
            my($estat) = $19;
            if($estat =~ /pe/) { $percent = 100; }
            elsif($estat =~ /br/) { $percent = 80; }
            elsif($estat =~ /bl/) { $percent = 40; }
            elsif($estat =~ /em/) { $percent = 20; }
            elsif($estat =~ /De/) { $percent = 10; }
            &main::run("/echo -W hpbar \"Enemy" . sprintf("(%7.7s):", defined $enemy?$enemy:"unknown") . &gauge(10,0,100,$percent) . "\"");
        } else {
            $incombat=0;
            &main::run("/echo -W hpbar \"Enemy(none):            \"");
        }
        if(defined $12) {
            &main::run("/echo -W hpbar \"" . $filledColor . "          Armor          " . $windowColor . "\"");
        } else {
            &main::run("/echo -W hpbar \"" . $emptyColor .  "          Armor          " . $windowColor . "\"");
        }
        if(defined $13) {
            &main::run("/echo -W hpbar \"" . $filledColor . "          Shield         " . $windowColor . "\"");
        } else {
            &main::run("/echo -W hpbar \"" . $emptyColor .  "          Shield         " . $windowColor . "\"");
        }
        if(defined $14) {
            &main::run("/echo -W hpbar \"" . $filledColor . "        Stoneskin        " . $windowColor . "\"");
        } else {
            &main::run("/echo -W hpbar \"" . $emptyColor .  "        Stoneskin        " . $windowColor . "\"");
        }
        if(defined $15) {
            &main::run("/echo -W hpbar \"" . $filledColor . "       Minor Globe       " . $windowColor . "\"");
            $mgup = 1;
        } else {
            &main::run("/echo -W hpbar \"" . $emptyColor .  "       Minor Globe       " . $windowColor . "\"");
            $mgup = 0;
        }
        if(defined $16 || defined $17) {
            if(defined $16) {
                &main::run("/echo -W hpbar \"" . $filledColor . "  Protection from Evil   " . $windowColor . "\"");
            } else {
                &main::run("/echo -W hpbar \"" . $filledColor . "  Protection from Good   " . $windowColor . "\"");
            }
        } else {
            &main::run("/echo -W hpbar \"" . $emptyColor .  "  Protection from Stuff  " . $windowColor . "\"");
        }
        if($cnc > 50 && $lastcnc <= 50 && $haveelemental) { main::run("say elemental, begone"); }
        if($cnc > 80) { main::run("say all, begone"); }
        $lastcnc = $cnc;
        if($incombat) {
            if($sps > 300 && !$slowed) { main::run("cast slow"); }
            elsif($sps > 400) { main::run("cast acid arrow"); }
        }
    }
#    if(/^  --> ((Something|Boring beetle|[A-Za-z]+ golem|[A-Za-z]+ elemental)+:[ABCDE~!+]+(, )?)*.*/) {
    if(/^  --> (.*)$/ || ($linectr == 2 && /^      (.*)$/)) {
        $summons = $1;
        if($summons =~ /elemental/) { 
            $haveelemental = 1; 
        } else { $haveelemental = 0; }
        if($summons =~ /Something/) { 
            $havephantasm = 1; 
        } else { $havephantasm = 0; }
        if($incombat) {
            if($cnc == 0 && $mgup && $sps > 300 && !$haveelemental) { 
                main::run("cast monster summoning 3"); 
            }
            elsif($sps > 350 && !$havephantasm) { 
                main::run("cast phantasmal killer"); 
            }
        }
    }
}

&main::run("/hook -F -T OUTPUT -p 100 check_hpbar = /run -Lperl ThreeKingdoms::check_hpbar");
&main::run("/window -w25 -h10 -x-0 -y4 -B -t0 -c31 hpbar");
# Why doesn't this work?
# from perl you need 4 backslashes where you want ONE to appear.  *sigh*
&main::run('/hook -T SEND -Ft\'^kill\\\\s+(\\\\w+)\' grabkill = phk;bt;/eval $ThreeKingdoms::enemy = $1');
if(!defined $enemy) { $enemy = ""; } # don't want it to be undef.
&main::run('/hook -T SEND -Ft\'^tk\\\\s+(\\\\w+)\' grabtk = /eval $ThreeKingdoms::enemy = $1;');
&main::run("/hook -p 10000 -T SEND -C (kill|tk) dontkilldraal = /run -Lperl ThreeKingdoms::dontkilldraal");
sub dontkilldraal {
    if(/^(?:kill|tk)\s(\w+)\s*$/) {
        if($1 =~ /draal/) {
            main::run("/echo \"${main::red}####################### DON'T KILL DRAAL!!!! #############################${main::off}\"");
            return 1;  # This isn't a fallthrough so returning 1 will cause it to stop here.
        }
    }
    return 0;
}

%opposite_dir = ( 
    'n' => 's',
    'ne' => 'sw',
    'e' => 'w',
    'se' => 'nw',
    's' => 'n',
    'sw' => 'ne',
    'w' => 'e',
    'nw' => 'se',
    'u' => 'd',
    'd' => 'u'
);

# A simple bouncer bot:
my($bounce_dir, $minhps, $minhppct, $meditating) = (0, 0, 0, 0);
sub main::dirtcmd_bounce {
    @_ = split / /; # MCL likes to pass parameters in $_, need to change this...
    unless($_[0]) {
        if($bounce_dir) { 
            $bounce_dir = 0; 
            print "\@\@ Bouncing disabled.\n";
        } else {
            print "\@\@ bounce syntax: \n";
            print "\@\@\tbounce <direction> <min hp>\n";
            print "\@\@\tWill bounce a mob that is to the <direction> of you\n";
        }
        return;
    }
    $bounce_dir = $_[0];
    if($_[1]) {
        if($_[1] =~ /^([0-9]+)(%)?$/) {
            $minhps = (defined $2)?0:$1;
            $minhppct = (defined $2)?$1:0;
        } else {
            print "Invalid syntax for bounce\n";
        }
    }
    unless($meditating) { &main::dirt_send($bounce_dir); }
}

#&main::hook_add("output", "ThreeKingdoms::bounce_grabber", 
&main::run("/hook -T OUTPUT -p -1 bounce_grabber = /run -Lperl ThreeKingdoms::bounce_grabber");
sub bounce_grabber {
    if(/^You stop meditating, feeling fully refreshed\.$/) { 
        &main::dirt_send("ls");
        &main::dirt_send("p");
        if($bounce_dir) {
            &main::dirt_send("p");
            &main::dirt_send($bounce_dir); 
        }
        $meditating = 0;
    }
    if(/^You sit down and close your eyes to begin meditating\.$/) {
        $meditating = 1;
    }
    if(/^You stop meditating\.$/) {
        $meditating = 0;
    }
    if(m#^ HP: ([0-9]+)/([0-9]+)#) { 
        if($bounce_dir) {
            unless($minhps) { $minhps = $2*($minhppct/100.0); }
            if($1 <= $minhps && !$meditating) { 
                &main::dirt_send($opposite_dir{$bounce_dir});
                &main::dirt_send("med");
            }
        }
#        unless($meditating) { &main::dirt_send("electrify"); }
    }
}

# Nexus system.
#
# This allows you to get from point A to point B by just typing the location 
# name of point B.  What it does is 'glance' at the room first.  If it 
# recognizes the room description (a key in %nexi), it constructs a command of
# the form A-B where A and B are the names of locations A (where you are) and B
# (where you want to go).  YOU must define an alias A-B containing the path.
# If A-B is not an alias, it will look for all aliases of the form A-X, and X-B
# and send both of them.



print "Loaded ThreeKingdoms module\n";
1;
