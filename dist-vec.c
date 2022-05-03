/** File name: dist-vec.c
    Description: implementation of distance vector routing protocol
    Date: 21/04/2020
    Maker: Jaehoon Jeong, pauljeong@skku.edu
*/

#include "dist-vec.h"

net_table_entry* g_net_table; //Network address table in which a router has subnet addresses
int g_net_table_size; //size of g_net_table

fw_table_entry* g_fw_table; //Forwarding table which is used for forwarding packets
int g_fw_table_size; //size of g_fw_table

rt_table_entry* g_rt_table; //Routing table
int g_rt_table_size; //size of g_rt_table

port_table_entry* g_port_table; //Port table
int g_port_table_size; //size of g_port_table


int dv_init_tables(in_addr_t* addr, int* mask, int addr_num, int* sock)
{ //initialize g_rt_table, g_net_table, and g_fw_table with the router's network information
  char itf_prefix[4] = "eth"; //prefix of interface name
  in_addr_t net_addr; //network address
  char buf[10]; //buffer for interface serial number
  int i;

  /* allocate memory for the routing table, g_rt_table */
  g_rt_table = (rt_table_entry*) calloc(RT_TABLE_SIZE, sizeof(rt_table_entry));
  if(g_rt_table == NULL)
  {
    perror("g_rt_table cannot be allocated memory");
    exit(1);
  }

  /* allocate memory for the network address table, g_net_table */
  g_net_table = (net_table_entry*) calloc(NET_TABLE_SIZE, sizeof(net_table_entry));
  if(g_net_table == NULL)
  {
    perror("g_net_table cannot be allocated memory");
    exit(1);
  }

  /* allocate memory for the forwarding table, g_fw_table */
  g_fw_table = (fw_table_entry*) calloc(FW_TABLE_SIZE, sizeof(fw_table_entry));
  if(g_fw_table == NULL)
  {
    perror("g_fw_table cannot be allocated memory");
    exit(1);
  }
  
  /* allocate memory for the port table, g_port_table */
  g_port_table = (port_table_entry*) calloc(PORT_TABLE_SIZE, sizeof(port_table_entry));
  if(g_port_table == NULL)
  {
    perror("g_port_table cannot be allocated memory");
    exit(1);
  }

  g_rt_table_size = 0;
  g_net_table_size = 0;
  g_fw_table_size = 0;
  g_port_table_size = 0;

  for(i = 0; i < addr_num; i++)
  {
    net_addr = addr[i] & mask[i]; //get network address through netmasking

    /* add a new dv table entry to g_rt_table */
    g_rt_table[i].dest = net_addr;
    g_rt_table[i].mask = mask[i];
    g_rt_table[i].next = 0;
    g_rt_table[i].hop = 1;
    g_rt_table[i].itf = sock[i];
    strcpy(g_rt_table[i].itf_name, itf_prefix);
    sprintf(buf, "%d", i);
    strcat(g_rt_table[i].itf_name, buf);
    g_rt_table[i].status = RTE_UP;
    g_rt_table[i].time = 0; //there is no timeout for the address of the local subnet attached to the router's interface
    g_rt_table_size++;

    /* add a new subnet address to g_net_table */
    g_net_table[i].net = net_addr;
    g_net_table[i].mask = mask[i];
    g_net_table_size++;

    /* add a new forwarding information to g_fw_table */
    g_fw_table[i].dest = net_addr;
    g_fw_table[i].mask = mask[i];
    g_fw_table[i].next = 0;
    g_fw_table[i].itf = sock[i];
    strcpy(g_fw_table[i].itf_name, g_rt_table[i].itf_name);
    g_fw_table[i].flag = 1;
    g_fw_table_size++;

    /* add a new interface port to g_port_table */
    g_port_table[i].itf = sock[i];
    strcpy(g_port_table[i].itf_name, g_rt_table[i].itf_name);
    g_port_table_size++;
  }

  return 0;
}

char* dv_get_itf_name(int sock)
{ //return the interface name corresponding to incoming socket (sock)
  char* ptr = NULL;
  int i;

  for(i = 0; i < g_port_table_size; i++)
  {
    if(sock == g_port_table[i].itf)
    {
      ptr = g_port_table[i].itf_name;
      break;
    }
  }
  
  return ptr;
}

int dv_broadcast_dv_message(in_addr_t* myipaddrs)
{ //broadcast the routing information with DV exchange message

  /** FILL IN YOUR CODE */
  /* 1. constuct a DV message containing the pairs of the subnet address and netmask by using its routing table 

     2. broadcast the DV message to every active network interface
  *****************************************************************/

#ifdef _DEBUG
  //printf("dv_broadcast_dv_message(): the router broadcasts ite routing information\n");
#endif


  ushort len;
  char net_addr[200]="";
  for(int i=0;i<g_rt_table_size;i++){
    if(g_rt_table[i].status==RTE_DOWN)
      continue;
    struct in_addr net;
    net.s_addr = g_rt_table[i].dest;
    strcat(net_addr, inet_ntoa(net));
    char mask[6];
    sprintf(mask,"%d",g_rt_table[i].mask);
    char hop[3];
    sprintf(hop,"%d",g_rt_table[i].hop);
    strcat(net_addr, "/");
    strcat(net_addr, mask);
    strcat(net_addr, "/");
    strcat(net_addr, hop);
    strcat(net_addr, "\n");
  }
  strcat(net_addr,"x");
  strcat(net_addr,"0");
  strcat(net_addr,"\0");
  len=strlen(net_addr);
  in_addr_t dst = IP_BCASTADDR;

  // for(int j=0;j<g_port_table_size;j++){
  //   printf("here^^\n");
  //   sendmessage(g_port_table[j].itf, myipaddrs[0], dst, len, DATA_DV, net_addr);//dvmsg send
  // }

  for(int j=0;j<g_fw_table_size;j++){
    if(g_fw_table[j].flag==(-1))
      continue;

    // printf("id is %d, here^^\n",j);
    sendmessage(g_fw_table[j].itf, myipaddrs[0], dst, len, DATA_DV, net_addr);//dvmsg send
  }

  // printf("BROADCASTED NORMAL DV_MSG\n");

  return 1;   
}

int dv_broadcast_dv_message_for_link_breakage(int sock, in_addr_t* myipaddrs) //broadcast the routing information with DV exchange message containing the network address related to the link which is broken due to a hub crash.
{
  /** FILL IN YOUR CODE */
  /* 1. change the state of the network entry related to sock (or hub) from RTE_UP to RTE_DOWN and set the corresponding forwarding entry's flag to 0

     2. constuct a DV message containing the pairs of the subnet address and netmask of the unreachable network related to sock 

     3. broadcast the DV message to every active network interface
  *****************************************************************/
  int down_sock=sock;
  ushort len;
  char net_addr[200]="";
  for(int i=0;i<g_rt_table_size;i++){
    if(g_rt_table[i].itf==down_sock){
      struct in_addr net;
      net.s_addr = g_rt_table[i].dest;
      strcat(net_addr, inet_ntoa(net));
      char mask[6];
      sprintf(mask,"%d",g_rt_table[i].mask);
      char hop[3];
      sprintf(hop,"%d",g_rt_table[i].hop);
      strcat(net_addr, "/");
      strcat(net_addr, mask);
      strcat(net_addr, "/");
      strcat(net_addr, hop);
      strcat(net_addr, "\n");
      break;
    }
  }
  strcat(net_addr,"x");
  strcat(net_addr,"1");
  strcat(net_addr,"\0");
  len=strlen(net_addr);
  in_addr_t dst = IP_BCASTADDR;
  // in_addr_t dst;
  // g_port_table_size

  for(int i=0;i<g_rt_table_size;i++){
    if(g_rt_table[i].itf==sock){
      // printf("BEFORE : g_rt_table[%d].status = %d\n",i, g_rt_table[i].status);
      g_rt_table[i].status=RTE_DOWN;
      // printf("AFTER : g_rt_table[%d].status = %d\n",i, g_rt_table[i].status);
    }
  }
  // printf("update rt for link breakage\n");

  for(int i=0;i<g_fw_table_size;i++){
    if(g_fw_table[i].itf==sock){
      // printf("BEFORE : g_fw_table[%d].flag = %d\n",i, g_fw_table[i].flag);
      g_fw_table[i].flag=-1;
      // printf("AFTER : g_fw_table[%d].flag = %d\n",i, g_fw_table[i].flag);
    }
  }
  // printf("update fw for link breakage\n");

  for(int j=0;j<g_port_table_size;j++){
    if(g_port_table[j].itf!=down_sock){
      // printf("down sock is %d and g_port_table[%d].itf is %d\n",down_sock, j, g_port_table[j].itf);
      // printf("RIGHT BEFORE SENDMESSAGE : down sock is %d and g_port_table[%d].itf is %d\n",down_sock, j, g_port_table[j].itf);
      sendmessage(g_port_table[j].itf, myipaddrs[0], dst, len, DATA_DV, net_addr);//dvmsg send
    }
  }

  // printf("before return\n");

  return 1;
}

int dv_update_rt_table(int sock, in_addr_t neighbor, dv_entry* dv, int dv_entry_num)
{ //update g_rt_table with dv entries sent by neighbor which are reachable network via the neighbor
  in_addr_t net_addr1, net_addr2; //network addresses
  int flag1 = 0; //flag to see if neighbor's network address is the same network
                 //as the network address of the incoming interface of the router

  int flag2 = 0; //flag to see if the new dv_entry exists in g_dv_table
  int i, j, k; //loop index

  for(i = 0; i < dv_entry_num; i++) //for-1
  {
      for(j = 0; j < g_net_table_size; j++) //for-2
      {
        net_addr1 = dv[i].dest & dv[i].mask;
        net_addr2 = g_net_table[j].net & g_net_table[j].mask;
       
        if(net_addr1 == net_addr2) //since dv[i].dest is the address of the network directly attached to the router, the router ignores it.
	{
          flag1 = 1;
	  break;
	}
      } //end of for-2

      if(flag1 == 1)
      {
	flag1 = 0;
	continue;
      }

      for(k = 0; k < g_rt_table_size; k++) //for-3
      {
	if(dv[i].dest == g_rt_table[k].dest)
	{
          flag2 = 1; /* 2005-12-5: flag2=1 indicates that the destination entry is in routing table and should be updated without adding the destination entry to the routing table again. */

          /* 2005-12-6: update the time of the corresponding routing entry since the neighbor towards the destination is alive.
             The condition that neighbor == g_rt_table[k].next says that the advertising neighbor is the same neighbor as the existing next hop of the routing entry */
          if((g_rt_table[k].status == RTE_UP) && (neighbor == g_rt_table[k].next))
	  {
            g_rt_table[k].time = getcurtime(); //get the current time and update the time field to prevent the expiration of the entry
          }
          /* 2005-12-5: If the status of the destination entry is RTE_DOWN, the new dv entry should be substituted for the destination entry regardless of the distance */
          else if((g_rt_table[k].status == RTE_DOWN) || ((dv[i].hop + 1) < g_rt_table[k].hop)) //update the hop count and next hop to the destination network
	  {
            g_rt_table[k].next = neighbor;
            g_rt_table[k].hop = dv[i].hop + 1;
            g_rt_table[k].status = RTE_UP;
            g_rt_table[k].time = getcurtime(); //get the current time and update the time fieldg_rt_table[k].time = getcurtime(); //get the current time and update the time field
	    break;
	  }
	}    
      } //end of for-3

      if(flag2 == 0) //there is not the new dv_entry dv[i] in g_rt_table
      {
        g_rt_table[g_rt_table_size].dest = dv[i].dest;
        g_rt_table[g_rt_table_size].mask = dv[i].mask;
        g_rt_table[g_rt_table_size].next = neighbor;
        g_rt_table[g_rt_table_size].hop = dv[i].hop + 1;
        g_rt_table[g_rt_table_size].itf = sock;
        strcpy(g_rt_table[g_rt_table_size].itf_name, dv_get_itf_name(sock)); //copy the corresponding interface name into itf_name
        g_rt_table[g_rt_table_size].status = RTE_UP;
        g_rt_table[g_rt_table_size].time = getcurtime(); //get the current time and update the time field

        g_rt_table_size++;
      }
      else
        flag2 = 0; //reset flag2 to 0
      
  } //end of for-1 

  return 1;
}

int dv_update_rt_table_for_link_breakage(int sock, in_addr_t neighbor, dv_entry* dv, int dv_entry_num)
{ //update g_rt_table with dv entry sent by neighbor which becomes unreachable network due the link breakage (e.g., hub is down)
  /** FILL IN YOUR CODE */
  /** 1. set the "status" of the routing entry corresponding to the network related to the broken link
         to "RTE_DOWN".


      sock을 기준으로 break하는게 아니라, dv_entry를 보고 break 해야지!
   *************************************************************/
  // printf("dv_entry_num %d\n",dv_entry_num);
  for(int i=0;i<dv_entry_num;i++){
    for(int j=0;j<g_rt_table_size;j++){
      if(g_rt_table[j].dest==dv[i].dest){
        g_rt_table[j].status=RTE_DOWN;
      }

      if(g_fw_table[j].dest==dv[i].dest){
        g_fw_table[j].flag=-1;
      }
    }
  }
  return 1;
}

int dv_update_fw_table()
{ //update forwarding table (g_fw_table) with the routing table (g_rt_table)

  /** FILL IN YOUR CODe */
  /** 1. update g_fw_table with g_rt_table
         - if an entry corresponding to a network address exists, update the "flag" of the forwarding entry
         - otherwise, add a new forwarding entry to g_fw_table.
 
  *************************************** */
  int i,j,check;
  for(i=0;i<g_rt_table_size;i++){
    check=0;

    for(j=0;j<g_fw_table_size;j++){
      if(g_rt_table[i].dest==g_fw_table[j].dest){
        check=1;
        break;//next rt_table entry
      }
    }

    if(check==0){
      g_fw_table[g_fw_table_size].dest=g_rt_table[i].dest;
      g_fw_table[g_fw_table_size].mask=g_rt_table[i].mask;
      g_fw_table[g_fw_table_size].next=g_rt_table[i].next;
      g_fw_table[g_fw_table_size].itf=g_rt_table[i].itf;
      strncpy(g_fw_table[g_fw_table_size].itf_name, g_rt_table[i].itf_name,ITF_NAME_SIZE);
      g_fw_table[g_fw_table_size].flag=1;
      g_fw_table_size++;
    }
  }
  return 1;
}

int dv_update_routing_info(int sock, char* dat, int dat_len, in_addr_t src)
{ //update routing table and forwarding table
  int ret_val;
  dv_entry* dv;
  int dv_entry_num;
  ushort cmd; //DV message command = {DV_ADVERTISE, DV_BREAKAGE}

  /** FILL IN YOUR CODE */
  /** 1. convert the DV exchange message into dv_entry arrary
        - allocate the memory for dv
        - set dv_entry_num to the actual number of DV entries contained in DV exchange message

      2. set cmd to the command described in DV exchange message
  ************************************************************/  
  int i=0;
  dv_entry_num=0;
  while(dat[i]!='x'){
    if(dat[i]=='\n')
      dv_entry_num++;
    i++;
  }
  dv=(dv_entry*)malloc(sizeof(dv_entry)*dv_entry_num);
  char temp[20];
  i=0;
  int entry_id=0;

  while(dat[i]!='x'){
    char tmp_addr[20]="\0";
    char tmp_mask[6]="\0";
    char tmp_hop[5]="\0";
    int tmp_hop_len=0;
    while(dat[i]!='/n' && dat[i]!='x'){
      int j=0; 
      while(dat[i]!='/'){
        //addr
        tmp_addr[j++]=dat[i++];
      }
      j=0;
      i++;
      while(dat[i]!='/'){
        tmp_mask[j++]=dat[i++];
      }
      j=0;
      i++;
      tmp_hop_len=0;
      while(dat[i]!='\n'){
        tmp_hop[j++]=dat[i++];
      }
      i++;
      
      struct in_addr dvNet;
      inet_aton(tmp_addr,&dvNet);
      dv[entry_id].dest=dvNet.s_addr;
      dv[entry_id].mask=atoi(tmp_mask);
      dv[entry_id].hop=atoi(tmp_hop);
      entry_id++;
      //ONE ENTRY DONE
    }
  }

  dv_entry_num=entry_id;

  //ADVERTISE? OR BREAKAGE?
  i++;
  char tmp[2]="";
  tmp[0]=dat[i];

  if(!strncmp(tmp,"0",1))
    cmd=DV_ADVERTISE;
  else if(!strncmp(tmp,"1",1)){
    cmd=DV_BREAKAGE;
  }

  // printf("cmd %hd\n",cmd);
  if(cmd == DV_ADVERTISE)
  {
    ret_val = dv_update_rt_table(sock, src, dv, dv_entry_num);
    if(ret_val != 1)
    {
      printf("dv_update_routing_info(): the DV exchange message has not been processed well\n");
      free(dv);
      return 0;
    }
  }
  else if(cmd == DV_BREAKAGE)
  {
    /** FILL IN YOUR CODE in dv_update_rt_table_for_link_breakage() function */
    printf("LINK BREAKAGE\n");
    ret_val = dv_update_rt_table_for_link_breakage(sock, src, dv, dv_entry_num);
    if(ret_val != 1)
    {
      printf("dv_update_routing_info(): the DV exchange message has not been processed well\n");
      free(dv);
      return 0;
    }
  }
  else
  {
    printf("dv_update_routing_info(): DV command (%d) is invalid!\n", cmd);
    free(dv);
    return 0;
  }

  /** FILL IN YOUR CODE in dv_update_fw_table() */
  ret_val = dv_update_fw_table();
  if(ret_val != 1)
  {
    printf("dv_update_routing_info(): the forwarding table has not been updated well\n");
    free(dv);
    return 0;
  }

  free(dv);
  return 1;
} 

void dv_update_tables_for_timeout(long curtime, int myconfigint)
{ //update the routing table and forwarding table for timeout

  /** FILL IN YOUR CODE */
  /** update its routing table and forwarding table on the focus of "time" field in routing table entry  

      1. If the difference (= current time - routing entry's time) > myconfigint, then the routing entry should be disabled with state RTE_DOWN

      2. disable the corresponding forwarding table entry
  ***********************************************************/

}

int dv_forward(IPPkt* ippkt)
{ //forward the packet to the appropriate next hop router or host

#ifdef _DEBUG  
  struct in_addr src, dst;
  char buf[50];
  char src_addr[16];
  char dst_addr[16];

  src.s_addr = ippkt->src;
  dst.s_addr = ippkt->dst;
  strncpy(src_addr, inet_ntoa(src), sizeof(src_addr)); 
  strncpy(dst_addr, inet_ntoa(dst), sizeof(dst_addr)); 
  // printf("The router should forward the IP packet (source: %s, destination: %s)\n", src_addr, dst_addr);
#endif

  /** FILL IN YOUR CODE */
  /**
      1. find the MAC address of the next hop using the forwarding table and ARP function

      2. replace the previous destination MAC address of the Ethernet frame with the new MAC address 

      3. send the Ethernet frame to the appropriate hub
  **************************************************************************************************/
  int sock=-1;
  sock=dv_get_sock_for_destination(sock, ippkt->src, ippkt->dst);
  sendippkt(sock, ippkt);
  return 1;
}

void dv_show_routing_table()
{ //show routing table
  /** FILL IN YOUR CODE for show the routing table */
  if(g_rt_table_size>0){
    printf("THIS IS THE ROUTING TABLE\n");
    printf("         NetAddr     | Mask | Hop | Next_HOP | Interface | STATUS\n");
    for(int i=0;i<g_rt_table_size;i++){
      struct in_addr dst, next;
      char dst_addr[16];
      char next_addr[16];
      dst.s_addr = g_rt_table[i].dest;
      next.s_addr = g_rt_table[i].next;
      strncpy(dst_addr, inet_ntoa(dst), sizeof(dst_addr));
      strncpy(next_addr, inet_ntoa(next), sizeof(next_addr)); 
      
      printf("Entry %d : %s | %d | %d | %s | %d | %d\n",i+1,dst_addr,g_rt_table[i].mask,g_rt_table[i].hop,next_addr,g_rt_table[i].itf,g_rt_table[i].status);
    }

  }
  else{
    printf("NO ENTRIES IN ROUTING TABLE YET\n");
  }
}

void dv_show_forwarding_table()
{ //show forwarding table
  /** FILL IN YOUR CODE for show the forwarding table */
  if(g_fw_table_size>0){
    printf("THIS IS THE FORWARDING TABLE\n");
    printf("         NetAddr | Mask | NextHop | Interface | Flag\n");

    for(int i=0;i<g_fw_table_size;i++){

      struct in_addr dst, next;
      char dst_addr[16];
      char next_addr[16];
      dst.s_addr = g_fw_table[i].dest;
      next.s_addr = g_fw_table[i].next;
      strncpy(dst_addr, inet_ntoa(dst), sizeof(dst_addr));
      strncpy(next_addr, inet_ntoa(next), sizeof(next_addr)); 

      printf("Entry %d : %s | %d | %s | %d | %d\n",i+1,dst_addr,g_fw_table[i].mask,next_addr,g_fw_table[i].itf, g_fw_table[i].flag);
    }
  }
  else{
    printf("NO ENTRIES IN FORWARDING TABLE YET\n");
  }
}

int dv_get_sock_for_destination(int sock, in_addr_t src, in_addr_t dst)
{ //return an appropriate socket for destination address dst with the forwarding table
  //if dst address is BROADCAST, 

#ifdef _DEBUG
  // printf("\nthe router should determine which socket is used for sending the IP packet to the appropriate next hop with the forwarding table\n");
#endif

  /** FILL IN YOUR CODE */
  /** 1. find the socket for next hop related to the given destination dst from the forwarding table
      2. return the socket
    
  **********************************************/
  sock=-1;
  int i=0;
  
  for(i=0;i<g_fw_table_size;i++){
    if(((dst & g_fw_table[i].mask)==g_fw_table[i].dest)&&g_fw_table[i].flag==1){
      sock=g_fw_table[i].itf;
      return sock;
    }
  }
  return sock;
}

int dv_ipaddr_to_hwaddr(in_addr_t ippkt_dst, HwAddr ethpkt_dst)
{ //convert the dst IP address into next hop's MAC address

#ifdef _DEBUG
  printf("\nthe router should determine which MAC address is used for sending the Ethernet frame to the appropriate next hop with the forwarding table\n");
#endif

  /** FILL IN YOUR CODE */
  /** 1. find the MAC affress for next hop related to the given destination dst with ARP function
      2. return the MAC address through parameter ethpkt_dst
    
  **********************************************/

  HwAddr middle;
  in_addr_t next_hop;
  int i;
  for(i=0;i<g_fw_table_size;i++){
    if((g_fw_table[i].mask&ippkt_dst)==g_fw_table[i].dest){
      next_hop=g_fw_table[i].next;
      break;
    }
    else{
      // printf("index is %d, and no matching entry in forwarding table\n",i);
    }
  }

  if(g_fw_table[i].next==0){
    // printf("BEFORE arp_ipaddr_to_hwadrr : LAST LAN\n");
    arp_ipaddr_to_hwaddr(ippkt_dst, middle);
  }else{
    // printf("BEFORE arp_ipaddr_to_hwadrr : STILL MORE TO HOP\n");
    arp_ipaddr_to_hwaddr(next_hop, middle);
  }
  memcpy(ethpkt_dst, middle, sizeof(HwAddr));
  return 1;
}
