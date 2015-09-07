#
# mktool.pl
#
# $Id: mktool.pl 1.6 Broadcom SDK $
# 
# $Copyright: Copyright 2012 Broadcom Corporation.
# This program is the proprietary software of Broadcom Corporation
# and/or its licensors, and may only be used, duplicated, modified
# or distributed pursuant to the terms and conditions of a separate,
# written license agreement executed between you and Broadcom
# (an "Authorized License").  Except as set forth in an Authorized
# License, Broadcom grants no license (express or implied), right
# to use, or waiver of any kind with respect to the Software, and
# Broadcom expressly reserves all rights in and to the Software
# and all intellectual property rights therein.  IF YOU HAVE
# NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
# IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
# ALL USE OF THE SOFTWARE.  
#  
# Except as expressly set forth in the Authorized License,
#  
# 1.     This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof,
# and to use this information only in connection with your use of
# Broadcom integrated circuit products.
#  
# 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
# PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
# OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
# 
# 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
# INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
# ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
# TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
# THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
# WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
# ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$

use File::Path;
use File::Find;
use File::Copy;
use Cwd;

($prog = $0) =~ s/.*\///;

SWITCH:
{
    $op = shift;

    if ($op eq "-rm") { mktool_rm(@ARGV); last SWITCH; }
    if ($op eq "-cp") { mktool_cp(@ARGV); last SWITCH; }
    if ($op eq "-md") { mktool_md(@ARGV); last SWITCH; }
    if ($op eq "-ln") { mktool_ln(@ARGV); last SWITCH; }
    if ($op eq "-foreach") { mktool_foreach(@ARGV); last SWITCH; }
    if ($op eq "-dep") { mktool_makedep(@ARGV); last SWITCH; }
    if ($op eq "-echo") { mktool_echo(@ARGV); last SWITCH; }
    if ($op eq "-beep") { mktool_beep(@ARGV); last SWITCH; }
    die("$prog: unknown option '$op'\n");
}

exit 0;




#
# mktool_execute
#
# Executes a command, returns exist status. 
# Performs token special translation before execution. 
#

sub mktool_execute
{
    my $token = shift;
    my @cmds = @_;
    
#    printf("mktool_execute: token = '$token'\n");
    foreach $cmd (@cmds)
    {
	#printf("mktool_execute: cmd = '$cmd'\n");
	$cmd =~ s/\#\#/$token/g;
	if($cmd =~ /^-p/)
	{
	    $cmd =~ s/^-p//;
	    printf("$cmd\n");
	}
	else
	{
	    system($cmd);
	    my $excode = ($? >> 8);
	    exit $excode if $excode;
	}
    }
}


$find_regexp = "";
@find_cmd;
    
#
# mktool_foreach
#
sub mktool_foreach
{
    if($_[0] eq "-find")
    {
	shift;
	$find_dir = shift;
	$find_regexp = shift;
	@find_cmds = @_;
	
	if(!($find_dir =~ /^\//))
	{
	    $find_dir = cwd() . "/" . $find_dir;
	}
	find(\&_mktool_foreach_find_wanted, $find_dir);
    }
    else
    {
	my $subdir = 0;
	if($_[0] eq "-subdir")
	{
	    $subdir = 1;
	    shift;
	}

	my @thingies = split(' ', shift);
    
	foreach $thingy (@thingies)
	{
	    chdir $thingy unless $subdir == 0;
	    mktool_execute($thingy, @_);
	    chdir ".." unless $subdir == 0;
	}
    }
}



sub _mktool_foreach_find_wanted
{
    my $expr = "\$File::Find::name =~ /\^$find_regexp\$/";

    if(eval($expr))
    {
	mktool_execute($File::Find::name, @find_cmds);
	exit $excode if $excode;
    }
}


#
# rm
#
# Removes a list of objects
#
sub mktool_rm
{
    my($f);

    foreach $f (@_) {
	eval { rmtree($f) };
	if ($@) {
	    die "$prog $op: failed to remove $f: $@\n";
	}
    }
}

#
# md
#
# Makes a list of directories
#
sub mktool_md
{
    my($dir);

    foreach $dir (@_) {
	$dir =~ s!/+$!!;
	eval { mkpath($dir) };
	if ($@) {
	    die "$prog $op: failed to make directory $dir: $@\n";
        }
    }
}


sub mktool_cp
{
    my($from, $to) = @_;

    if (@_ != 2) {
	    die "$prog $op: must have two arguments\n";
    }
    copy($from, $to) ||
	die "$prog $op: failed to copy $from to $to: $!\n";
}

sub mktool_ln
{
    my($old, $new) = @_;

    if (@_ != 2) {
	    die "$prog $op: must have two arguments\n";
    }
    link ($old, $new) ||
	die "$prog $op: failed to link $new to $old: $!\n";
}


#	@echo "$@ \\" > ${BLDDIR}/$(notdir $@)
#	@if ($(DEPEND)) >> $(BLDDIR)/$(notdir $@); then	\
#	    exit 0;				\
#	else					\
#	    rm -f ${BLDDIR}/$(notdir $@);	\
#	    exit 1;				\
#	fi

#	$(MAKEDEP) "$@" "$(BLDDIR)/$(notdir $@)" "$(DEPEND)"

sub mktool_makedep
{
    my ($source, $target, $cmd, $curdir) = @_;
    my @result = `$cmd`;
    my $sdk = $ENV{'SDK'};
    my $count;
    my $tmp;
    local $resultant;

## Comman $cmd
#Command $cmd
print <<MKTOOL;

mktool.pl::
curdir $curdir

MKTOOL

    # derive the path to the target
    $curdir = "$sdk/$curdir";

    # save the basename
    $dirName = substr($target,0,rindex($target,"\/"));

    # prepare the top line of the %.d file
    $resultant="$source \\\n\${BLDDIR}/";
    if(!$?)
    {
	foreach $line (@result)
	{
	$line =~ s/^#.*\n//g;		# some makedeps produce comments
	$line =~ s/\n+$//;		# toss trailing newlines
	$line =~ s/(\s+)(\.\/)/  /g;    # remove leading ./

	# insert SDK path before ../
	$line =~ s/(\s+)(\.\.\/)/${1}$curdir\/${2}/g; 

	# insert SDK path
	$line =~ s/(\s+)(\w+)/${1}$curdir\/${2}/g; 

	$count=0;
        $tmp=$line;
	while( (index($line,"..")>-1) & ($count < 20) )
	{
		$line=~s/\/\w+\/\.\.//;
		# if we hit a major recursion, revert the line, report
		# this to the output and drop out of the loop, but do
		# continue, this should not halt generation
		if($count++>19)
		{ 
			print "mktool.pl: could not process $line \n\n";
			print ":: curdir  $curdir\n";
			print ":: target  $target\n";
			print ":: cmd     $cmd\n";
			$line=$tmp;
		}
	}

	# set all the paths to use the $SDK variable
	$line =~ s/$ENV{'SDK'}/\$\{SDK\}/g; 
	$resultant=$resultant . $line;
	}

	# some compilers return extra newlines
	$resultant=~s/\n//g;

	# now clean up the result
	$resultant=~s/\\/\\\n/g;

	mktool_md($dirName) unless (-d $dirName);
	open (TARGET, ">$target") ||
	    die("$prog $op: cannot open '$target' for writing: $!\n");
	print TARGET "$resultant\n";
	close(TARGET);
    }
}

sub mktool_echo
{
    print "@_\n";
}

sub mktool_beep
{
    -t STDOUT && defined $ENV{MAKEBEEP} && print "\007";
}
