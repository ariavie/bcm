/*
 * $Id: nd_protocols.c 1.7 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "nd_mm.h"
#include "nd_registers.h"
#include "nd_util.h"
#include "nd_debug.h"
#include "bcm_ces_sal.h"  /*BATMISTTFIX19.7.11*/

/* */
/* Related standards */
/*  */
/* RFC 768: UDP - UDP pseudoheader definiton when IPv4 used */
/* RFC 791: IPv4 */
/* RFC 2460: IPv6 - UDP pseudoheader definiton when IPv6 used */
/* RFC 1071: internet checksum */
/* RFC 1141, 1624: incremental checksum */
/* */
/*  */

/* */
/* */


typedef void (*AgNdStrictAddCb) (AgNdDevice	*p_device,
								 void		*p_data,		/* data to write to strict buffer */
								 AG_U32		n_size, 		/* the data size in bytes */
								 AG_U32		*p_addr);		/* on input contains start offset from the strict memory */


/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Add Ethernet header */
/* */
static void
ag_nd_header_add_eth(AgNdPacketHeader *p_hdr, AgNdBitwise *p_bw)
{
    AG_U32 i;

    assert(0 == p_bw->n_offset);

    for (i = 0; i < 6; i++)
        ag_nd_bitwise_write(p_bw, p_hdr->x_eth.a_destination[i], 8);

    for (i = 0; i < 6; i++)
        ag_nd_bitwise_write(p_bw, p_hdr->x_eth.a_source[i], 8);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds VLAN tag(s) */
/* */
static void
ag_nd_header_add_vlan(AgNdPacketHeader *p_hdr, AgNdBitwise *p_bw)
{
    AG_U32 i;

    assert(0 == p_bw->n_offset);

    for (i = 0; i < p_hdr->n_vlan_count; i++)
    {
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_ETH_VLAN, 16);
        ag_nd_bitwise_write(p_bw, p_hdr->a_vlan[i].n_priority, 3);
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_VLAN_CFI, 1);
        ag_nd_bitwise_write(p_bw, p_hdr->a_vlan[i].n_vid, 12);
    }
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds MPLS labes(s) */
/* */
static void
ag_nd_header_add_mpls(AgNdPacketHeader *p_hdr, AgNdBitwise *p_bw)
{
    AG_U32 i;

    assert(0 == p_bw->n_offset);

    for (i = 0; i < p_hdr->n_mpls_count; i++)
    {
        ag_nd_bitwise_write(p_bw, p_hdr->a_mpls[i].n_label, 20);
        ag_nd_bitwise_write(p_bw, p_hdr->a_mpls[i].n_expiremental, 3);
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_MPLS_BOS, 1);
        ag_nd_bitwise_write(p_bw, p_hdr->a_mpls[i].n_ttl, 8);
    }
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds VC labes */
/* */
static void
ag_nd_header_add_vc_label(AgNdPacketHeader *p_hdr, AgNdBitwise *p_bw)
{
    assert(0 == p_bw->n_offset);

    ag_nd_bitwise_write(p_bw, p_hdr->x_vc_label.n_label, 20);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_VCLABEL_EXP, 3);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_VCLABEL_BOS, 1);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_VCLABEL_TTL, 8);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds PTP header */
/* */
static void
ag_nd_header_add_ptp(
    AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    AG_U32 i;

    assert(0 == p_bw->n_offset);
    assert(((p_bw->p_addr - p_bw->p_buf) & 1) == 0);

    p_tx_hdr_fmt->x_fields.n_rtp_or_ptp_header_pointer = p_bw->p_addr - p_bw->p_buf;
    p_tx_hdr_fmt->x_fields.b_rtp_or_ptp_exists = AG_TRUE;

    ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.n_ts, 4);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_MESSAGE_TYPE, 4);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_RESERVED, 4); 
    ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.n_ver, 4);   

    if (p_hdr->x_ptp.b_dst_port) 
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_HDR_LEN + AG_ND_PROTO_PTP_TIMESTAMP_LEN + AG_ND_PROTO_PTP_DST_PORT_LEN, 16); /* long msg len */
    else
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_HDR_LEN + AG_ND_PROTO_PTP_TIMESTAMP_LEN, 16); /* short msg len */

    ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.n_domain, 8);    
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_RESERVED, 8); 
    ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.n_flags, 16);    

    for (i = 0; i < sizeof(p_hdr->x_ptp.a_correction); i++) /* 8 bytes */
         ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.a_correction[i], 8);    

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_RESERVED, 32);    

    for (i = 0; i < sizeof(p_hdr->x_ptp.a_src_port); i++) /* 10 bytes */
         ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.a_src_port[i], 8);    

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_SEQUENCE_ID, 16);    
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_CONTROL, 8);    
    ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.n_log_mean_interval, 8);    

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_TIMESTAMP, 32);    
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_TIMESTAMP, 32);    
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_PTP_TIMESTAMP, 16);    
    
    if (p_hdr->x_ptp.b_dst_port)
        for (i = 0; i < sizeof(p_hdr->x_ptp.a_dst_port); i++) /* 10 bytes */
             ag_nd_bitwise_write(p_bw, p_hdr->x_ptp.a_dst_port[i], 8);    

    assert(0 == p_bw->n_offset);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds CES header */
/* */
static void
ag_nd_header_add_ces(
    AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    assert(0 == p_bw->n_offset);
    assert(((p_bw->p_addr - p_bw->p_buf) & 1) == 0);

    p_tx_hdr_fmt->x_fields.n_ces_header_pointer = p_bw->p_addr - p_bw->p_buf;
    p_tx_hdr_fmt->x_fields.b_cep_or_ptp_sec_exists = AG_FALSE;

    ag_nd_bitwise_write(p_bw, 0, 16);                      /* CW placeholder */
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_CES_SQN, 16);

    assert(0 == p_bw->n_offset);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds RTP header */
/* */
static void
ag_nd_header_add_rtp(
    AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    assert(0 == p_bw->n_offset);

    if (!p_hdr->b_rtp_exists)
        return;

    p_tx_hdr_fmt->x_fields.n_rtp_or_ptp_header_pointer = p_bw->p_addr - p_bw->p_buf;
    p_tx_hdr_fmt->x_fields.b_rtp_or_ptp_exists = AG_TRUE;

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_VERSION, 2);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_PADDING, 1);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_EXTENTION, 1);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_CSRC_COUNT, 4);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_MARKER, 1);
    ag_nd_bitwise_write(p_bw, p_hdr->x_rtp.n_pt, 7);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_SQN, 16);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_RTP_TIMESTAMP, 32);
    ag_nd_bitwise_write(p_bw, p_hdr->x_rtp.n_ssrc, 32);

    assert(0 == p_bw->n_offset);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds IPv4 header */
/* */
static void
ag_nd_header_add_ipv4(
    AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    assert(0 == p_bw->n_offset);

    p_tx_hdr_fmt->x_fields.n_ipv4_header_pointer = p_bw->p_addr - p_bw->p_buf;
    p_tx_hdr_fmt->x_fields.b_ipv4_exists = AG_TRUE;

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_VERSION, 4);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_HDR_LEN / 4, 4);
    ag_nd_bitwise_write(p_bw, p_hdr->x_ip4.n_tos, 8);

    assert(0 == p_bw->n_offset);
    assert(((p_bw->p_addr - p_bw->p_buf) & 1) == 0);
    ag_nd_bitwise_write(p_bw, 0, 16);                              /* total length placeholder */

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_ID, 16);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_RSV, 1);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_DONT_FRAG, 1);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_MORE_FRAG, 1);
    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_FRAG_OFFSET, 13);
    ag_nd_bitwise_write(p_bw, p_hdr->x_ip4.n_ttl, 8);
	if((p_hdr->n_l2tpv3_count > 0)&&(!(p_hdr->x_l2tpv3.b_udp_mode)))
	{
		/*case we in L2TPV3 over IP next protcol need to be L2TPV3(0X0073). */
		ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_L2TPV3, 8); /* 0x73 Same for IPV6*/
	}
	else
	{
    	ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_PROTO, 8);
	}	

    assert(0 == p_bw->n_offset);
    assert(((p_bw->p_addr - p_bw->p_buf) & 1) == 0);
    ag_nd_bitwise_write(p_bw, 0, 16);                              /* header checksum placeholder */

    ag_nd_bitwise_write(p_bw, p_hdr->x_ip4.n_source, 32);
    ag_nd_bitwise_write(p_bw, p_hdr->x_ip4.n_destination, 32);

    assert(0 == p_bw->n_offset);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds IPv6 header */
/* */
static void
ag_nd_header_add_ipv6(
    AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    AG_U32  i;


    assert(0 == p_bw->n_offset);

    /* */
    /* the IPv4 header is used to store IPv6 header */
    /* */
    p_tx_hdr_fmt->x_fields.n_ipv4_header_pointer = p_bw->p_addr - p_bw->p_buf;
    p_tx_hdr_fmt->x_fields.b_ipv4_exists = AG_FALSE;

    ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP6_VERSION, 4);
    ag_nd_bitwise_write(p_bw, p_hdr->x_ip6.n_traffic_class, 8);
    ag_nd_bitwise_write(p_bw, p_hdr->x_ip6.n_flow_label, 20);

    assert(0 == p_bw->n_offset);
    assert(((p_bw->p_addr - p_bw->p_buf) & 1) == 0);
    ag_nd_bitwise_write(p_bw, 0, 16);                              /* payload length placeholder */

    /*ORI*/ /*ISTT 18.7.11*/
    if((p_hdr->n_l2tpv3_count > 0)&&(!(p_hdr->x_l2tpv3.b_udp_mode)))
    {
      /*case we in L2TPV3 over IP next protcol need to be L2TPV3(0X0073). */
       ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP4_L2TPV3, 8);
    }
    else
    {
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_IP6_NEXT_HEADER, 8);
    }	

    ag_nd_bitwise_write(p_bw, p_hdr->x_ip6.n_hop_limit, 8);

    for (i = 0; i < 16; i++)
        ag_nd_bitwise_write(p_bw, p_hdr->x_ip6.a_source[i], 8);

    for (i = 0; i < 16; i++)
        ag_nd_bitwise_write(p_bw, p_hdr->x_ip6.a_destination[i], 8);

    assert(0 == p_bw->n_offset);
}

/*/////////////////////////////////////////////////////////////////////////////////////////// */
/* */
/* Adds UDP header */
/* */
static void
ag_nd_header_add_udp(
    AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    assert(0 == p_bw->n_offset);

    p_tx_hdr_fmt->x_fields.n_udp_header_pointer = p_bw->p_addr - p_bw->p_buf;
    p_tx_hdr_fmt->x_fields.b_udp_exists = AG_TRUE;

    ag_nd_bitwise_write(p_bw, p_hdr->x_udp.n_source, 16);
    ag_nd_bitwise_write(p_bw, p_hdr->x_udp.n_destination, 16);

    assert(0 == p_bw->n_offset);
    assert(((p_bw->p_addr - p_bw->p_buf) & 1) == 0);
    ag_nd_bitwise_write(p_bw, 0, 16);                  /* datagram length placeholder */
    ag_nd_bitwise_write(p_bw, 0, 16);                  /* udp checksum placeholder */

    assert(0 == p_bw->n_offset);
}

/*ORI*/
/*add header L2TPV3 */
static void
ag_nd_header_add_l2tpv3(
	AgNdPacketHeader    *p_hdr, 
    AgNdBitwise         *p_bw)
{
    assert(0 == p_bw->n_offset);
	if(p_hdr->n_l2tpv3_count < 1)
	{
		return;
	}
    else
    {
    	if(p_hdr->x_l2tpv3.b_udp_mode)
    	{
    		/*case l2tpve over udp the packet include l2tp header */
        	ag_nd_bitwise_write(p_bw, p_hdr->x_l2tpv3.n_header, 32);
    	}
        ag_nd_bitwise_write(p_bw, p_hdr->x_l2tpv3.n_session_peer_id, 32);
		if(p_hdr->n_l2tpv3_count >= 2)
		{
			/*case cookie is 32 */
			ag_nd_bitwise_write(p_bw, p_hdr->x_l2tpv3.n_peer_cookie1, 32);
		}
		if(p_hdr->n_l2tpv3_count == 3)
		{
			/*case cookie is 64 */
			/*ag_nd_bitwise_write(p_bw, p_hdr->x_l2tpv3.n_cookie >> 32, 32); */
			/*ag_nd_bitwise_write(p_bw, (p_hdr->x_l2tpv3.n_cookie & 0xffffffff), 32); */
			ag_nd_bitwise_write(p_bw, p_hdr->x_l2tpv3.n_peer_cookie2, 32);
		}
    }
}
/*///////////////////////////////////////////////////////////////////////////// */
/* computes the UDP lentgh */
/*  */
void 
ag_nd_header_udp_length(
    AG_U16              *p_len,
    AgNdPacketHeader    *p_hdr)
{
	/*ORI*/
	/*changed to suport l2tpv3 over ip */
    /**p_len = AG_ND_PROTO_UDP_HDR_LEN; */

    if (AG_ND_ENCAPSULATION_PTP_IP == p_hdr->e_encapsulation)
    {
        /* */
        /* add PTP header length */
        /*  */
        *p_len += AG_ND_PROTO_PTP_HDR_LEN;  

        /* */
        /* add PTP body length */
        /*  */
        *p_len += AG_ND_PROTO_RTP_TIMESTAMP_LEN;    

        if (p_hdr->x_ptp.b_dst_port)
            *p_len += AG_ND_PROTO_RTP_PORT_LEN;
    }
    else
    {
        /* */
        /* add CES header length */
        /*  */
        *p_len += AG_ND_PROTO_CES_HDR_LEN;

        if (p_hdr->b_rtp_exists)
            *p_len += AG_ND_PROTO_RTP_HDR_LEN;
    }
	/*ORI*/
	/*add header length of l2tp  */
	if(p_hdr->n_l2tpv3_count > 0)
	{
		if(p_hdr->x_l2tpv3.b_udp_mode)
		{
			/*case we in l2tpv3 over udp */
			*p_len += AG_ND_PROTO_UDP_HDR_LEN;
			*p_len += 4;/*header of l2tpv3 only in l2tpv3 over udp */
		}
		if(p_hdr->n_l2tpv3_count == 1)
		{
			*p_len += 4;/*no cookie session_id */
		}
		else
		{
			if(p_hdr->n_l2tpv3_count == 2)
			{
				*p_len += 8;/*one cookie+session_id */
			}
			else
			{
				*p_len += 12;/*two cookie+session_id */
			}
		}
	}
	else
	{
		/*case udp without l2tpv3 */
		*p_len += AG_ND_PROTO_UDP_HDR_LEN;
	}
		
	
}


/*///////////////////////////////////////////////////////////////////////////// */
/* computes the UDP lentgh and checksum. */
/*  */
/* This function must be called after all the the protocol headers added to the */
/* bitwise buffer, i.e. after ag_nd_header_add_ces and ag_nd_header_add_rtp was  */
/* called. */
/*  */
/* UDP checksum doesn't includes UDP length and payload, but it does include the  */
/* const fields of upper level protocol headers (CES and RTP). */
/*  */
/* UDP checksum computed is not really a checksum, but only a 32 bit sum of the */
/* appropriate fields, the ag_nd_header_update will take care of the rest. */
/*  */
/* UDP length field doesn't includes the TDM payload size. */
/*  */
/*  */
static void
ag_nd_header_udp_length_and_checksum(
    AgNdBitwise         *p_bw,     
    AG_U32              *p_chksum,  /* checksum will be placed here */
    AG_U16              *p_len,     /* length will be placed here */
    AgNdPacketHeader    *p_hdr,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    AG_U8  *p;
    AG_U32 i;


    
    *p_chksum = 0; 
    *p_len = 0;
    

    /* */
    /* checksum */
    /* */
    /*ORI*/
	/*if we are in l2tpv3 over ip need to remove udp header */
    if((p_hdr->n_l2tpv3_count == 0)||(p_hdr->x_l2tpv3.b_udp_mode))
    {
    	for (p = p_bw->p_buf + p_tx_hdr_fmt->x_fields.n_udp_header_pointer;
         	p < p_bw->p_addr;
        	p += 2)
        	*p_chksum += AG_ND_MAKE16LH(*(p+1), *p);
    	if (4 == p_hdr->n_ip_version)
    	{
        	*p_chksum += AG_ND_LOW16(p_hdr->x_ip4.n_source);
        	*p_chksum += AG_ND_HIGH16(p_hdr->x_ip4.n_source);
        	*p_chksum += AG_ND_LOW16(p_hdr->x_ip4.n_destination);
        	*p_chksum += AG_ND_HIGH16(p_hdr->x_ip4.n_destination);
        	*p_chksum += AG_ND_PROTO_IP4_PROTO;
    	}
    	else
    	{
        	for (i = 0; i < 16; i += 2) 
        	{
            	*p_chksum += AG_ND_MAKE16LH(p_hdr->x_ip6.a_destination[i+1], p_hdr->x_ip6.a_destination[i]);
            	*p_chksum += AG_ND_MAKE16LH(p_hdr->x_ip6.a_source[i+1], p_hdr->x_ip6.a_source[i]);
        	}   

        	*p_chksum += AG_ND_PROTO_IP6_NEXT_HEADER;
    	}

    	/* */
    	/* length */
    	/* */
    	ag_nd_header_udp_length(p_len, p_hdr);
    }
}

/*///////////////////////////////////////////////////////////////////////////// */
/* computes the IPv6 lentgh */
/*  */
/* IPv6 length field doesn't includes the TDM payload length, the  */
/* ag_nd_header_update will take care to update this field. */
/*  */
void
ag_nd_header_ipv6_length(
    AG_U16              *p_len,             /* length will be placed here */
    AgNdPacketHeader    *p_hdr)
{
    *p_len = 
        AG_ND_PROTO_UDP_HDR_LEN + 
        (p_hdr->b_rtp_exists ? AG_ND_PROTO_RTP_HDR_LEN : 0) +
        AG_ND_PROTO_CES_HDR_LEN;
    /* */
    /* length */
    /*  */
	/*ORI*/
	/*add header length of l2tp  */
	if(p_hdr->n_l2tpv3_count > 0)
	{
		if(p_hdr->x_l2tpv3.b_udp_mode)
		{
			/*case we in l2tpv3 over udp */
			*p_len += 4;/*header of l2tpv3 only in l2tpv3 over udp */
		}
        else
        {
           *p_len -= AG_ND_PROTO_UDP_HDR_LEN; /*If l2tp and not udp */
        }
		if(p_hdr->n_l2tpv3_count == 1)
		{
			*p_len += 4;/*no cookie session_id */
		}
		else
		{
			if(p_hdr->n_l2tpv3_count == 2)
			{
				*p_len += 8;/*one cookie+session_id */
			}
			else
			{
				*p_len += 12;/*two cookie+session_id */
			}
		}
	}
}


/*///////////////////////////////////////////////////////////////////////////// */
/* computes the IPv4 lentgh and checksum */
/*  */
/* This function must be called after all the the IPv4 header added to the */
/* bitwise buffer, i.e. after ag_nd_header_add_ipv4 was called. */
/*  */
/* IP checksum computed is not really a checksum, but only a 32 bit sum of the */
/* appropriate fields, the ag_nd_header_update will take care of the rest. */
/*  */
static void
ag_nd_header_ipv4_length_and_checksum(
    AgNdBitwise         *p_bw,
    AG_U32              *p_chksum,      /* checksum will be placed here */
    AG_U16              *p_len,         /* length will be placed here */
    AgNdPacketHeader    *p_hdr,
    AgNdRegTransmitHeaderFormatDescriptor   *p_tx_hdr_fmt)
{
    AG_U8  *p;
	*p_chksum = 0; 
    *p_len = 0;


    /* */
    /* length */
    /*  */
    ag_nd_header_udp_length(p_len, p_hdr);
    *p_len += AG_ND_PROTO_IP4_HDR_LEN;


    /* */
    /* checksum */
    /*  */

    for (p = p_bw->p_buf + p_tx_hdr_fmt->x_fields.n_ipv4_header_pointer;
         p < p_bw->p_buf + p_tx_hdr_fmt->x_fields.n_ipv4_header_pointer + AG_ND_PROTO_IP4_HDR_LEN; 
         p += 2)
        *p_chksum += AG_ND_MAKE16LH(*(p+1), *p);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* Builds the ingress channel packet header  */
/*  */
/* Builds the complete packet header template (except the IP/UDP length and  */
/* checksum fields) and descriptor. */
/*  */
/* ag_nd_header_update_cw must be called afterwards in order to update the IP/UDP  */
/* length and checksum fields according to CES CW and DBA settings. */
/* */
void ag_nd_header_build(
    AgNdPacketHeader    *p_hdr,             /* contains field values for requested protocols */
                                            /* (protocols are determined according to  */
                                            /* e_encapsulation value) */
                                            
    AgNdBitwise         *p_bw,              /* raw header template will be build here */
                                            /* (allocated by caller, b_buf points to memory location  */
                                            /* of n_buf_size bytes) */

    AgNdRegTransmitHeaderFormatDescriptor
                        *p_tx_hdr_fmt,      /* header template descriptor will be build here */
                                            /* (allocated by caller) */

    AG_U32              *p_udp_chksum,      /* in the case of IP encapsulation type , IP/UDP  */
    AG_U16              *p_udp_len,         /* precalculated checksums (as specified by FPGA spec) */
    AG_U32              *p_ip_chksum,       /* will be placed here */
    AG_U16              *p_ip_len)
{
    assert(p_hdr);
    assert(p_bw);
    assert(p_bw->p_buf);
    assert(p_udp_chksum);
    assert(p_udp_len);
    assert(p_ip_chksum);
    assert(p_ip_len);

    assert(p_hdr->e_encapsulation >= AG_ND_ENCAPSULATION_FIRST && 
           p_hdr->e_encapsulation <= AG_ND_ENCAPSULATION_LAST);


    /* */
    /* initialize bitwise */
    /* */
    ag_nd_memset(p_bw->p_buf, 0, p_bw->n_buf_size);
    p_bw->p_addr = p_bw->p_buf;
    p_bw->n_offset = 0;


    /* */
    /* initialize headert format descriptor */
    /* */
    p_tx_hdr_fmt->n_reg[0] = 0;
    p_tx_hdr_fmt->n_reg[1] = 0;


    /* */
    /* build header */
    /* */
    ag_nd_header_add_eth(p_hdr, p_bw);
    ag_nd_header_add_vlan(p_hdr, p_bw);


    switch(p_hdr->e_encapsulation)
    {
    case AG_ND_ENCAPSULATION_MPLS:

        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_ETH_MPLS, 16);

        ag_nd_header_add_mpls(p_hdr, p_bw);
        ag_nd_header_add_vc_label(p_hdr, p_bw);

        ag_nd_header_add_ces(p_hdr, p_bw, p_tx_hdr_fmt);
        ag_nd_header_add_rtp(p_hdr, p_bw, p_tx_hdr_fmt);

        break;


    case AG_ND_ENCAPSULATION_ETH:

        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_ETH_CES, 16);

        ag_nd_header_add_vc_label(p_hdr, p_bw);
        ag_nd_header_add_ces(p_hdr, p_bw, p_tx_hdr_fmt);
        ag_nd_header_add_rtp(p_hdr, p_bw, p_tx_hdr_fmt);

        break;

    case AG_ND_ENCAPSULATION_PTP_EHT:
        ag_nd_bitwise_write(p_bw, AG_ND_PROTO_ETH_PTP, 16);
        ag_nd_header_add_ptp(p_hdr, p_bw, p_tx_hdr_fmt);

        break;

    case AG_ND_ENCAPSULATION_IP:
    case AG_ND_ENCAPSULATION_PTP_IP:

        if (4 == p_hdr->n_ip_version)
        {
            ag_nd_bitwise_write(p_bw, AG_ND_PROTO_ETH_IP4, 16);
            ag_nd_header_add_ipv4(p_hdr, p_bw, p_tx_hdr_fmt);
        }
        else
        {
            ag_nd_bitwise_write(p_bw, AG_ND_PROTO_ETH_IP6, 16);
            ag_nd_header_add_ipv6(p_hdr, p_bw, p_tx_hdr_fmt);
        }

		/*ORI*/
		/*if l2tpv3 over ip we remove udp. */
		if((p_hdr->n_l2tpv3_count == 0)||(p_hdr->x_l2tpv3.b_udp_mode))
		{
        	ag_nd_header_add_udp(p_hdr, p_bw, p_tx_hdr_fmt);
		}
        if (AG_ND_ENCAPSULATION_IP == p_hdr->e_encapsulation)
        {
        	/*ORI*/
			/*add header l2tpv3 */
			if(p_hdr->n_l2tpv3_count == 0)
			{
				ag_nd_header_add_rtp(p_hdr, p_bw, p_tx_hdr_fmt);
            	ag_nd_header_add_ces(p_hdr, p_bw, p_tx_hdr_fmt);
			}
			else
			{
				/*if l2tp need to change l2tpv3,ces,rtp */
				ag_nd_header_add_l2tpv3(p_hdr, p_bw);
				ag_nd_header_add_ces(p_hdr, p_bw, p_tx_hdr_fmt);	
            	ag_nd_header_add_rtp(p_hdr, p_bw, p_tx_hdr_fmt);
			}	
        }
        else
        {
            ag_nd_header_add_ptp(p_hdr, p_bw, p_tx_hdr_fmt);
        }

        if (4 == p_hdr->n_ip_version)
            ag_nd_header_ipv4_length_and_checksum(
                p_bw, 
                p_ip_chksum,
                p_ip_len,
                p_hdr, 
                p_tx_hdr_fmt);
        else
            ag_nd_header_ipv6_length(
                p_ip_len,
                p_hdr);

		if((p_hdr->n_l2tpv3_count == 0)||(p_hdr->x_l2tpv3.b_udp_mode))
		{
        ag_nd_header_udp_length_and_checksum(
            p_bw, 
            p_udp_chksum,
            p_udp_len,
            p_hdr, 
            p_tx_hdr_fmt);
		}


        /* */
        /* For CAS ingress traffic HW is unable to update UDP checksum, so we  */
        /* set the UDP header exists field of template descriptor to zero. */
        /* */
        /* Still, the UDP header pointer is set set to UDP header offset, thus  */
        /* indicating IP and UDP headers presence for SW. */
        /*  */
        if (!p_hdr->b_udp_chksum)
            p_tx_hdr_fmt->x_fields.b_udp_exists = AG_FALSE;

        break;
    default:
        break;    
    }

    assert(0 == p_bw->n_offset);
}


/*///////////////////////////////////////////////////////////////////////////// */
/* Updates the UDP length and checksum fields */
/*  */
static void
ag_nd_header_update_udp(
    AgNdDevice          *p_device, 
    AgNdMemUnit         *p_unit,            /* header template's memory unit */
    AG_BOOL             b_update_chksum,    /* leave checksum as is if false */
    AG_U32              n_chksum,           /* precalculated UDP checksum */
    AG_U16              n_len,              /* precalculated UDP length */
    AG_U32              n_header_ptr,       /* header template's offset from beginning of memory unit */
    AG_U16              n_payload_size,     /* actual payload size */
    AG_U16              n_cw)               /* actual CES CW value: set to zero when updating PTP header */
{
    AG_U16  n_chksum16;


    /*  */
    /* update the UDP length field */
    /*  */
    n_len += n_payload_size;

    ag_nd_mem_write(p_device, p_unit, n_header_ptr + 4, &n_len, 1);

    if (b_update_chksum)
    {
        /* */
        /* update the UDP checksum */
        /* */
        n_chksum += n_cw;
    
        /* */
        /* UDP checksum includes the UDP length twice: from the UDP header and from  */
        /* the UDP pseudoheader */
        /* */
        n_chksum += n_len;
        n_chksum += n_len;
    
        while (n_chksum >> 16)
           n_chksum = (n_chksum & 0xffff) + (n_chksum >> 16);
    
        n_chksum16 = (AG_U16)n_chksum;
    
        ag_nd_mem_write(p_device, p_unit, n_header_ptr + 6, &n_chksum16, 1);
    }
}

/*///////////////////////////////////////////////////////////////////////////// */
/* Updates the IPv4/v6 length and checksum fields */
/*   */
static void
ag_nd_header_update_ip(
    AgNdDevice          *p_device, 
    AgNdMemUnit         *p_unit,
    AG_U32              n_chksum,
    AG_U16              n_len,
    AG_U32              n_header_ptr,
    AG_U16              n_payload_size,
    AG_BOOL             b_ipv4)
{
    AG_U16  n_chksum16;

    if (b_ipv4)
    {
        /* */
        /* update the IPv4 length field */
        /* */
        n_len += n_payload_size;
        ag_nd_mem_write(p_device, p_unit, n_header_ptr + 2, &n_len, 1);

        /* */
        /* update the IPv4 checksum field */
        /* */
        n_chksum += n_len;

        while (n_chksum >> 16)
           n_chksum = (n_chksum & 0xffff) + (n_chksum >> 16);

        n_chksum16 = (AG_U16)n_chksum;
        ag_nd_mem_write(p_device, p_unit, n_header_ptr + 10, &n_chksum16, 1);
    }
    else
    {
        /* */
        /* update the IPv6 length field */
        /* */
        n_len += n_payload_size;
        ag_nd_mem_write(p_device, p_unit, n_header_ptr + 4, &n_len, 1);
    }
}

/*///////////////////////////////////////////////////////////////////////////// */
/* Performs dynamic update of CES CW. Updates the IP/UDP checksum and */
/* payload length fields (payload length may change if DBA is un use). */
/*  */
void 
ag_nd_header_update_cw(
    AgNdDevice              *p_device,
    AgNdMemUnit             *p_mem_unit,
    AG_U32                  n_offset,
    AG_U16                  n_set_cw_mask,
    AG_U16                  n_set_cw_value,
    AgNdChannelIngressInfo  *p_channel_info)
{
    AG_U32  n_hdr_template_ptr;
    AG_U16  n_tdm_payload_size;
    AG_U16  n_ces_length;


    n_hdr_template_ptr = n_offset + 4; 


    /* */
    /* update CW flags */
    /*  */
    p_channel_info->n_cw &= ~n_set_cw_mask;
    p_channel_info->n_cw |= n_set_cw_value;

    /* */
    /* compute the TDM payload size: if L bit is set and the channel is in  */
    /* DBA mode, switch to zero length TDM payload */
    /* */
    if ((p_channel_info->n_cw & AG_ND_CES_CW_L) && p_channel_info->b_dba)
        n_tdm_payload_size = 0;
    else
        n_tdm_payload_size = p_channel_info->n_payload_size;


    if (p_channel_info->b_mef_len_support)
    {
        /* */
        /* update CES length field: includes CES CW (32 bits), RTP header (if exists and  */
        /* follows CES header) and CES payload. If the resulted length if greater or equal */
        /* to 42 it must be set to 0 */
        /*  */

        n_ces_length = 4;                           
        n_ces_length += n_tdm_payload_size;         
        
        if (!p_channel_info->x_tx_hdr_fmt.x_fields.b_udp_exists && 
             p_channel_info->x_tx_hdr_fmt.x_fields.b_rtp_or_ptp_exists)
        {
            /* */
            /* in the case of CESoE (identified by absence of UDP layer) RTP header follows */
            /* CES CW and therefore considered as CES payload */
            /* */
            n_ces_length += 12;
        }
        
        if (n_ces_length >= 42)
        {
            n_ces_length = 0;
        }
    }
    else
    {
        n_ces_length = 0;
    }



    p_channel_info->n_cw &= 0xffc0;
    p_channel_info->n_cw |= n_ces_length;

    /* */
    /* write down the new CW value */
    /* */
    ag_nd_mem_write(
        p_device, 
        p_mem_unit, 
        n_hdr_template_ptr + p_channel_info->x_tx_hdr_fmt.x_fields.n_ces_header_pointer, 
        &(p_channel_info->n_cw),
        1);

    /* */
    /* update the IP/UDP headers if in use */
    /* */
    if (p_channel_info->x_tx_hdr_fmt.x_fields.n_udp_header_pointer)
    {
        /* */
        /* UDP header is present (we use n_udp_header_pointer instead of  */
        /* b_udp_exists because the last one must be set to 0 for CAS */
        /* packets although UDP is present). Update IP length and header  */
        /* checksum (for IPv4) */
        /*  */
        ag_nd_header_update_ip(
            p_device,
            p_mem_unit,
            p_channel_info->n_ip_chksum,
            p_channel_info->n_ip_len,
            n_hdr_template_ptr + p_channel_info->x_tx_hdr_fmt.x_fields.n_ipv4_header_pointer,
            n_tdm_payload_size,
            AG_TRUE == p_channel_info->x_tx_hdr_fmt.x_fields.b_ipv4_exists);

        ag_nd_header_update_udp(
            p_device,
            p_mem_unit,
            p_channel_info->x_tx_hdr_fmt.x_fields.b_udp_exists, 
            p_channel_info->n_udp_chksum,
            p_channel_info->n_udp_len,
            n_hdr_template_ptr + p_channel_info->x_tx_hdr_fmt.x_fields.n_udp_header_pointer,
            n_tdm_payload_size,
            p_channel_info->n_cw);
    }
	else
	{
		/*ORI*/
		/*ADD CASE L2TPV3 OVER IP */
		if(p_channel_info->x_tx_hdr_fmt.x_fields.n_ipv4_header_pointer > 0)
		{
			/*case l2tpv3 over ip */
			ag_nd_header_update_ip(
            p_device,
            p_mem_unit,
            p_channel_info->n_ip_chksum,
            p_channel_info->n_ip_len,
            n_hdr_template_ptr + p_channel_info->x_tx_hdr_fmt.x_fields.n_ipv4_header_pointer,
            n_tdm_payload_size,
            AG_TRUE == p_channel_info->x_tx_hdr_fmt.x_fields.b_ipv4_exists);
		}
		
	}
}


/*///////////////////////////////////////////////////////////////////////////// */
/* Performs dynamic update PTP header. Updates the IP/UDP checksum and */
/* payload length fields. */
/*  */
void 
ag_nd_header_update_ptp(
    AgNdDevice          *p_device,
    AgNdMemUnit         *p_mem_unit,
    AG_U32              n_offset,
    AG_U32              n_udp_chksum, 
    AG_U16              n_udp_len,    
    AG_U32              n_ip_chksum,  
    AG_U16              n_ip_len)
{
    AgNdRegTransmitHeaderFormatDescriptor       x_tx_hdr_fmt;

    AG_U32  n_hdr_ptr;
    AG_U32  n_hdr_template_ptr;



    /* */
    /* read header format descriptor (4 bytes) */
    /* */
    n_hdr_ptr = n_offset; 
    n_hdr_template_ptr = n_hdr_ptr + 4; 
    ag_nd_mem_read(p_device, p_mem_unit, n_hdr_ptr, &(x_tx_hdr_fmt.n_reg), 2);


    /* */
    /* Update IP and UDP length and header checksum  */
    /*  */
    ag_nd_header_update_ip(
        p_device,
        p_mem_unit,
        n_ip_chksum,
        n_ip_len,
        n_hdr_template_ptr + x_tx_hdr_fmt.x_fields.n_ipv4_header_pointer,
        0,
        AG_TRUE == x_tx_hdr_fmt.x_fields.b_ipv4_exists);

    ag_nd_header_update_udp(
        p_device,
        p_mem_unit,
        x_tx_hdr_fmt.x_fields.b_udp_exists, 
        n_udp_chksum,
        n_udp_len,
        n_hdr_template_ptr + x_tx_hdr_fmt.x_fields.n_udp_header_pointer,
        0,
        0);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_strict_add */
/* */
/* ag_nd_strict_build helper: adds one protocol field to strict buffer at time */
/* */
static void 
ag_nd_strict_data_add(
    AgNdDevice  *p_device,
    void        *p_data,        /* data to write to strict buffer */
    AG_U32      n_size,         /* the data size in bytes */
    AG_U32      *p_addr)        /* on input contains start offset from the strict memory */
                                /* unit start, on output updated according to n_size */
{
    assert(0 == (n_size & 1));

    ag_nd_mem_write(p_device, p_device->p_mem_data_strict, *p_addr, p_data, n_size >> 1);
    *p_addr += n_size;
}

static void 
ag_nd_strict_ptp_add(
    AgNdDevice  *p_device,
    void        *p_data,        /* data to write to strict buffer */
    AG_U32      n_size,         /* the data size in bytes */
    AG_U32      *p_addr)        /* on input contains start offset from the strict memory */
                                /* unit start, on output updated according to n_size */
{
    assert(0 == (n_size & 1));

    ag_nd_mem_write(p_device, p_device->p_mem_ptp_strict, *p_addr, p_data, n_size >> 1);
    *p_addr += n_size;
}


void ag_nd_strict_update_vlan_id(AgNdDevice* 	p_device,
								 AG_U32 		n_vlan_count,
							     AgNdProtoVlan* p_vlan,
							     AgNdChannel 	n_channel_id,
							     AG_BOOL		b_ptp,
							     AG_U32* 		p_addr)
{
	AG_U32 i;
	AG_U32 n_addr;
	AG_U16 n_16bits;
	AgNdStrictAddCb ag_nd_strict_add = (b_ptp) ? ag_nd_strict_ptp_add 
											   : ag_nd_strict_data_add;
	
	assert(p_device);
	assert(p_vlan);

	if (p_addr == NULL)
	{
		n_addr = n_channel_id * AG_ND_RPC_STRICT_MAX;
		n_addr += 6; /* skip MAC address  */
		p_addr = &n_addr;
	}

	for (i = 0; i < n_vlan_count; i++)
    {
        n_16bits = ag_hton16((AG_U16)p_vlan->n_vid);
        ag_nd_strict_add(p_device, &n_16bits, 2, p_addr);
	    p_vlan++;
    }
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_strict_build */
/* */
/* Builds channel's strict data buffer in FPGA memory */
/* */
void ag_nd_strict_build(
    AgNdDevice          *p_device, 
    AgNdPacketHeader    *p_header, 
    AgNdChannel         n_channel_id)
{
    AG_U32  n_addr;
    AG_U16  n_16bits;
    AG_U32  n_32bits;
    /*AG_U32  i; BCM not used*/	
	AG_BOOL b_ptp;

	/*ORI*/
	/*add l2tpv3 cookie */

	AgNdStrictAddCb ag_nd_strict_add;
	
    assert(p_device);
    assert(p_header);
    assert(p_header->e_encapsulation >= AG_ND_ENCAPSULATION_FIRST && 
           p_header->e_encapsulation <= AG_ND_ENCAPSULATION_LAST);

	switch (p_header->e_encapsulation)
	{
		case AG_ND_ENCAPSULATION_PTP_EHT:
		case AG_ND_ENCAPSULATION_PTP_IP:
			b_ptp = AG_TRUE;
			ag_nd_strict_add = ag_nd_strict_ptp_add;
			break;
		default:	
			b_ptp = AG_FALSE;
			ag_nd_strict_add = ag_nd_strict_data_add;
			break;
	}
	
    /* */
    /* channel's strict buffer offset from the memory unit start */
    /* */
    n_addr = n_channel_id * AG_ND_RPC_STRICT_MAX;


    /* */
    /* source MAC address */
    /* */
    if (AG_ND_ENCAPSULATION_ETH == p_header->e_encapsulation ||
		AG_ND_ENCAPSULATION_PTP_EHT == p_header->e_encapsulation)
    {
        ag_nd_strict_add(p_device, p_header->x_eth.a_source, 6, &n_addr);
    }
    else
    {
        n_16bits = 0;
        ag_nd_strict_add(p_device, &n_16bits, 2, &n_addr);
        ag_nd_strict_add(p_device, &n_16bits, 2, &n_addr);
        ag_nd_strict_add(p_device, &n_16bits, 2, &n_addr);
    }


    /* */
    /* VLAN IDs */
    /*  */
	ag_nd_strict_update_vlan_id(p_device, 
								p_header->n_vlan_count, 
								p_header->a_vlan, 
								n_channel_id, 
								b_ptp,
								&n_addr);
	
    /* */
    /* source IP address */
    /*  */
    if (AG_ND_ENCAPSULATION_IP == p_header->e_encapsulation ||
		AG_ND_ENCAPSULATION_PTP_IP == p_header->e_encapsulation)
    {
        if (4 == p_header->n_ip_version)
        {
            n_32bits = ag_hton32(p_header->x_ip4.n_source);
            ag_nd_strict_add(p_device, &n_32bits, 4, &n_addr);
        }
        else
        {
            ag_nd_strict_add(p_device, p_header->x_ip6.a_source, 16, &n_addr);
        }
    }


    if (AG_ND_ENCAPSULATION_PTP_EHT == p_header->e_encapsulation ||
		AG_ND_ENCAPSULATION_PTP_IP == p_header->e_encapsulation)
    {
		n_16bits = (p_header->x_ptp.n_domain << 8);
		ag_nd_strict_add(p_device, &n_16bits, 2, &n_addr);		
    }
	
	/*ORI*/
	/*case l2tp need to add cookie to strict */
	if(p_header->n_l2tpv3_count > 0)
	{
		n_32bits = 0;
		if(p_header->n_l2tpv3_count >= 2)
		{
			n_32bits =  ag_hton32(p_header->x_l2tpv3.n_local_cookie1);
			ag_nd_strict_add(p_device, &n_32bits, 4, &n_addr);	
		}
		if(p_header->n_l2tpv3_count == 3)
		{
			n_32bits = 0;
			n_32bits =  ag_hton32(p_header->x_l2tpv3.n_local_cookie2);
			ag_nd_strict_add(p_device, &n_32bits, 4, &n_addr);
		}
	}
	/* */
    /* fill the rest of the strict buffer with zeros */
    /*  */
    n_16bits = 0;
    while (n_addr < ((AG_U32)n_channel_id + 1) * AG_ND_RPC_STRICT_MAX)
    {
        ag_nd_strict_add(p_device, &n_16bits, 2, &n_addr);
    }
}

