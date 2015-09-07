#!/usr/bin/perl
#
# $Id: stripfix.pl,v 1.2 Broadcom SDK $
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
#
# Perl version of the "stripfix.c" program
#
# In Tornado tools, stripppc and objcopyppc corrupt the ELF program header
# and require the use of the "stripfix" kludge.  Alternately, Solaris'
# strip works on PPC binaries also (/usr/ccs/bin/strip).
#

use Fcntl;

($prog = $0) =~ s,.*/,,;

($#ARGV == 3) ||
    die "Usage: $prog <unstripped-file> <stripped-file> <byte-offset> <word-count>\n" .
        "\tThis utility extracts <word-count> words from <unstripped-file>\n" .
        "\tstarting at offset <byte-offset>, and pastes them into the same\n" .
        "\tlocation in <stripped-file>.\n";

($unstripped, $stripped, $offset, $words) =
    ($ARGV[0], $ARGV[1], $ARGV[2], $ARGV[3]);

#
# Read the interesting bytes from the unstripped input file
#
sysopen(INFILE, $unstripped, O_RDONLY, 0) ||
    die "$prog: input file $unstripped: cannot open: $!\n";
binmode INFILE;
sysseek(INFILE, $offset, SEEK_SET) ||
    die "$prog: input file $unstripped: seek failed: $!\n";
$len = sysread(INFILE, $data, $words * 4);
defined($len) ||
    die "$prog: input file $unstripped: read failed: $!\n";
($len == $words * 4) ||
    die "$prog: input file $unstripped: read failed: returned $len bytes\n";
close(INFILE) ||
    die "$prog: input file $unstripped: close failed: $!\n";

#
# Now overwite the corresponding bytes in the stripped output file
#
sysopen(OUTFILE, $stripped, O_WRONLY, 0) ||
    die "$prog: output file $stripped: cannot open: $!\n";
binmode OUTFILE;
sysseek(OUTFILE, $offset, SEEK_SET) ||
    die "$prog: output file $stripped: seek failed: $!\n";
$len = syswrite(OUTFILE, $data, $words * 4);
defined($len) ||
    die "$prog: output file $stripped: write failed: $!\n";
($len == $words * 4) ||
    die "$prog: output file $stripped: write failed: only $len bytes written\n";
close(OUTFILE) ||
    die "$prog: output file $stripped: close failed: $!\n";

exit 0;
