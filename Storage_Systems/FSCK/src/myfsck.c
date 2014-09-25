#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* for memcpy() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include "ext2_fs.h"
#include "genhd.h"

#if defined(__FreeBSD__)
#define lseek64 lseek
#endif

/* linux: lseek64 declaration needed here to eliminate compiler warning. */
extern int64_t lseek64(int, int64_t, int);

const unsigned int sector_size_bytes = 512;

unsigned int partition_offset;

unsigned int block_size;

unsigned int lost_found_inode_no = 0;

unsigned int lost_found_flag = 2;

__u32 lost_found_offset = 0;

unsigned int link_count_flag = 0;

unsigned int block_count_flag = 0;

unsigned int counter = 0;

unsigned int groups = 0;

static int device;  /* disk file descriptor */

void print_sector (unsigned char *buf)
{
    int i;
    for (i = 0; i < sector_size_bytes; i++) {
        printf("%02x", buf[i]);
        if (!((i+1) % 32))
            printf("\n");      /* line break after 32 bytes */
        else if (!((i+1) % 4))
            printf(" ");   /* space after 4 bytes */
    }
}

void read_sectors (int64_t start_sector, unsigned int num_sectors, void *into)
{
    ssize_t ret;
    int64_t lret;
    int64_t sector_offset;
    ssize_t bytes_to_read;

    if (num_sectors == 1) {
//        printf("Reading sector %"PRId64"\n", start_sector);
    } else {
//        printf("Reading sectors %"PRId64"--%"PRId64"\n",
//               start_sector, start_sector + (num_sectors - 1));
    }

    sector_offset = start_sector * sector_size_bytes;

    if ((lret = lseek64(device, sector_offset, SEEK_SET)) != sector_offset) {
        fprintf(stderr, "Seek to position %"PRId64" failed: "
                "returned %"PRId64"\n", sector_offset, lret);
        exit(-1);
    }

    bytes_to_read = sector_size_bytes * num_sectors;

    if ((ret = read(device, into, bytes_to_read)) != bytes_to_read) {
//        fprintf(stderr, "Read sector %"PRId64" length %d failed: "
//                "returned %"PRId64"\n", start_sector, num_sectors, ret);
        	exit (-1);
		exit(-1);
    }
}

struct partition_custom
{	
	int part_no;
	struct partition temp;
	struct partition_custom *next;
};

int power_of_3_or_5(int x)
{
	int y = x;
	if (x==0 | x==1)
		return 1;
	while(x % 3 == 0)	
		x /= 3;
	if (x==1)
		return 1;
	while(y % 5 == 0)
		y /= 5;
	if (y==1)
		return 1;
	return 0;
}

void get_super_and_group(int device, struct ext2_super_block **super, struct ext2_group_desc **group)
{
	int i, group_flag = 1, flag = 0, no_of_groups = 0;
	super[0] = malloc(sizeof(struct ext2_super_block));
	group[0] = malloc(sizeof(struct ext2_group_desc));
	lseek(device, partition_offset+1024, SEEK_SET);
	read(device, super[0], sizeof(struct ext2_super_block));
	
//	printf("\nmagic %x", super[0]->s_magic);
	lseek(device, partition_offset+1024+sizeof(struct ext2_super_block), SEEK_SET);
	read(device, group[0], sizeof(struct ext2_group_desc));
	
	no_of_groups = (super[0]->s_blocks_count / super[0]->s_blocks_per_group);
	if ((super[0]->s_blocks_count % super[0]->s_blocks_per_group) != 0)
		no_of_groups += 1;

	for (i=1; i<no_of_groups; i++)
	{
		if (power_of_3_or_5(i))
		{
			super[flag+1] = malloc(sizeof(struct ext2_super_block));
			
			lseek(device, partition_offset + 1024 + (super[0]->s_blocks_per_group * (1024 << super[0]->s_log_block_size)), SEEK_SET);
			read(device, super[flag+1], sizeof(struct ext2_super_block));
			
//			lseek(device, partition_offset + 1024 + (super[flag]->s_blocks_per_group * (1024 << super[flag]->s_log_block_size)) + sizeof(struct ext2_super_block) + (32*(flag+1)), SEEK_SET);
//			read(device, group[flag+1], sizeof(struct ext2_group_desc));
			
			flag++;
		}
	}
	
	groups = no_of_groups;

	for (i=1; i<no_of_groups; i++)
	{
		lseek(device, partition_offset+1024+sizeof(struct ext2_super_block)+(i * sizeof(struct ext2_group_desc)), SEEK_SET);
		group[i] = malloc(sizeof(struct ext2_group_desc));		
		read(device, group[i], sizeof(struct ext2_group_desc));	
	}
	
}

int offset_inode(int inode_no, struct ext2_group_desc **group_descriptor, struct ext2_super_block **super)
{
	int group = 0;
	__u32 overall_offset = partition_offset;
//	printf("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
	while (inode_no > super[0]->s_inodes_per_group)
	{
//		printf("\nINODE %d", inode_no);
/*			if (!power_of_3_or_5(group))
			{
				group++;
				continue;
			}
*/			//overall_offset += (super[0]->s_blocks_per_group * (1024 << super[0]->s_log_block_size));
			inode_no -= super[0]->s_inodes_per_group;
			group++;
	}
//	printf("\nGROUP %d", group);
	overall_offset += (group_descriptor[group]->bg_inode_table * (1024 << super[0]->s_log_block_size)) + ((inode_no - 1) * super[0]->s_inode_size);
	return overall_offset;	
}

void get_directory(int, int, int, int, struct ext2_group_desc**, struct ext2_dir_entry_2, struct ext2_inode, struct ext2_super_block**, unsigned char*, unsigned char*, unsigned char*, unsigned char*, unsigned char*, unsigned char*);

void get_inode(int device, int parent_inode, int child_inode, int links_count, struct ext2_group_desc **group_descriptor, struct ext2_super_block **super, unsigned char *inode_bitmap, unsigned char *local_inode_bitmap, unsigned char *block_bitmap, unsigned char *local_block_bitmap, unsigned char *link_count, unsigned char *local_link_count)
{
	struct ext2_inode inode;
	struct ext2_dir_entry_2 directory;
	lseek(device, offset_inode(child_inode, group_descriptor, super), SEEK_SET);
	read(device, &inode, sizeof(struct ext2_inode));
	if (link_count_flag == 1)
		link_count[child_inode-1] = inode.i_links_count;
	get_directory(device, parent_inode, child_inode, links_count, group_descriptor, directory, inode, super, inode_bitmap, local_inode_bitmap, block_bitmap, local_block_bitmap, link_count, local_link_count);
	}

void get_directory(int device, int parent_inode, int child_inode, int links_count, struct ext2_group_desc **group_descriptor, struct ext2_dir_entry_2 directory, struct ext2_inode inode, struct ext2_super_block **super, unsigned char *inode_bitmap, unsigned char *local_inode_bitmap, unsigned char *block_bitmap, unsigned char *local_block_bitmap, unsigned char *link_count, unsigned char *local_link_count)
{
	int i = 0, k = 0, j = 0, l = 0, temp = 2, group = 0, pointed_to_block = 0, indirect_block_no = 0, double_indirect_block_no = 0, triple_indirect_block_no = 0, temp_block;
	unsigned char local_bit, bitmap_byte, local_bitmap_byte;
	struct ext2_dir_entry_2 lost_found_directory, temp_directory;
	while (i <= 14)
	{		
		__u32 overall_offset = partition_offset;
		if ((inode.i_mode & 0xF000) != 0x4000 & i == 12)
		{
			if (inode.i_block[i] != 0 & block_count_flag == 1)
			{	
//				printf("\nBLOCK %d", inode.i_block[i]);				
				local_block_bitmap[inode.i_block[i]] = 1;
				counter++;

				overall_offset += (inode.i_block[i] * (1024 << super[0]->s_log_block_size)); 			
//				lseek(device, overall_offset, SEEK_SET);
//				read(device, &pointed_to_block, sizeof(int));
				j = 0;
				while(j < ((1024 << super[0]->s_log_block_size) / sizeof(int)))
				{			
					lseek(device, partition_offset + (inode.i_block[i] * (1024 << super[0]->s_log_block_size)) + (j*sizeof(int)), SEEK_SET);
					read(device, &indirect_block_no, sizeof(int));
//					printf("\nIndirect block no from 12 %d j %d", indirect_block_no, j);
					if (indirect_block_no != 0 & block_count_flag == 1)
					{	
						local_block_bitmap[indirect_block_no] = 1;
						counter++;
					}
					if (indirect_block_no == 0)
						break;
					else
						j++;
				}
			}
			if (inode.i_block[i] == 0)
				break;
			else
			{
				i++;
				continue;
			}	
		}
				
					
		else if ((inode.i_mode & 0xF000) != 0x4000 & i == 13)
		{
			if (inode.i_block[i] != 0 & block_count_flag == 1)
			{	
//				printf("\nBLOCK %d", inode.i_block[i]);				
				local_block_bitmap[inode.i_block[i]] = 1;
				counter++;
				overall_offset += (inode.i_block[i] * (1024 << super[0]->s_log_block_size)); 			
//				lseek(device, overall_offset, SEEK_SET);
//				read(device, &pointed_to_block, sizeof(int));
				j = 0;
				while(j < ((1024 << super[0]->s_log_block_size) / sizeof(int)))
				{			
					lseek(device, partition_offset + (inode.i_block[i] * (1024 << super[0]->s_log_block_size)) + (j*sizeof(int)), SEEK_SET);
					read(device, &indirect_block_no, sizeof(int));
//					printf("\nIndirect block no from 13 %d", indirect_block_no);
					if (indirect_block_no != 0 & block_count_flag == 1)
					{
						local_block_bitmap[indirect_block_no] = 1;
						counter++;
						k = 0;
						while(k < ((1024 << super[0]->s_log_block_size) / sizeof(int)))
						{					
							lseek(device, partition_offset + (indirect_block_no * (1024 << super[0]->s_log_block_size)) + (k*sizeof(int)), SEEK_SET);
							read(device, &double_indirect_block_no, sizeof(int));
//							printf("\ndouble Indirect block no from 13 %d", double_indirect_block_no);
							if (double_indirect_block_no != 0 & block_count_flag == 1)
							{
								local_block_bitmap[double_indirect_block_no] = 1;
								counter++;
							}
							if (double_indirect_block_no == 0)
								break;
							else
								k++;
						}
					}
					if (indirect_block_no == 0)
						break;
					else
						j++;
				}
			}
			if (inode.i_block[i] == 0)
				break;
			else
			{
				i++;
				continue;
			}
		}

		else if ((inode.i_mode & 0xF000) != 0x4000 & i == 14)
		{
			if (inode.i_block[i] != 0 & block_count_flag == 1)
			{	
//				printf("\nBLOCK %d", inode.i_block[i]);				
				local_block_bitmap[inode.i_block[i]] = 1;
				counter++;
				overall_offset += (inode.i_block[i] * (1024 << super[0]->s_log_block_size)); 			
//				lseek(device, overall_offset, SEEK_SET);
//				read(device, &pointed_to_block, sizeof(int));
				j = 0;
				while(j < ((1024 << super[0]->s_log_block_size) / sizeof(int)))
				{			
					lseek(device, partition_offset + (inode.i_block[i] * (1024 << super[0]->s_log_block_size)) + (j*sizeof(int)), SEEK_SET);
					read(device, &indirect_block_no, sizeof(int));
//					printf("\nIndirect block no from 14 %d", indirect_block_no);
					if (indirect_block_no != 0 & block_count_flag == 1)
					{
						local_block_bitmap[indirect_block_no] = 1;
						counter++;
						k = 0;
						while(k < ((1024 << super[0]->s_log_block_size) / sizeof(int)))
						{					
							lseek(device, partition_offset + (indirect_block_no * (1024 << super[0]->s_log_block_size)) + (k*sizeof(int)), SEEK_SET);
							read(device, &double_indirect_block_no, sizeof(int));
//							printf("\ndouble Indirect block no from 14 %d", double_indirect_block_no);
							if (double_indirect_block_no != 0 & block_count_flag == 1)
							{
								local_block_bitmap[double_indirect_block_no] = 1;
								counter++;
								l = 0;
								while(l < ((1024 << super[0]->s_log_block_size) / sizeof(int)))
								{
									lseek(device, partition_offset + (double_indirect_block_no*(1024<<super[0]->s_log_block_size))+(l*sizeof(int)), SEEK_SET);
									read(device, &triple_indirect_block_no, sizeof(int));
//								printf("\ntriple Indirect block no from 12 %d", triple_indirect_block_no);
									if (triple_indirect_block_no != 0 & block_count_flag == 1)
									{	
										local_block_bitmap[triple_indirect_block_no] = 1;
										counter++;
									}
									if (triple_indirect_block_no == 0)
										break;
									else
										l++;
								}
							}
							if (double_indirect_block_no == 0)
								break;
							else
								k++;
						}
					}
					if (indirect_block_no == 0)
						break;
					else
						j++;
				}
			}
			if (inode.i_block[i] == 0)
				break;
			else
			{
				i++;
				continue;
			}
		}
		else
		{	
			overall_offset += (inode.i_block[i] * (1024 << super[0]->s_log_block_size)); 
			lseek(device, overall_offset, SEEK_SET);
			read(device, &directory, sizeof(struct ext2_dir_entry_2));
		}		
		if (((inode.i_mode & 0xF000) == 0x4000))
		{

			if (directory.inode != 0)
			{
				if (block_count_flag == 1)
				{				
					local_block_bitmap[inode.i_block[i]] = 1;
					counter++;
//					printf("\nBLOCK %d inode no %d dir name %s", inode.i_block[i], directory.inode, directory.name);								
				}
				do
				{	
					if (strcmp(directory.name, "lost+found") == 0)
						lost_found_inode_no = directory.inode;
					if ((strcmp(directory.name, ".") == 0) & directory.inode == lost_found_inode_no)
					{
//						printf("\noffset printing %d", overall_offset);
						lost_found_offset = overall_offset;
					}			

				directory.name[directory.name_len] = '\0';
	
				if (strcmp(directory.name, ".") == 0)
					{	
						temp = directory.inode;
						if (directory.inode != child_inode)
						{
							printf("\nInode value for . is %d but supposed to be %d", directory.inode, child_inode);
							directory.inode = child_inode;
							lseek(device, overall_offset, SEEK_SET);
							write(device, &directory, sizeof(struct ext2_dir_entry_2));
						}
//						link_count++;
						if (link_count_flag == 1)
							local_link_count[directory.inode-1]++;
					}
					if ((strcmp(directory.name, "..") == 0))
					{
						if (directory.inode != parent_inode)
						{
							printf("\nInode value for .. is %d but supposed to be %d", directory.inode, parent_inode);
							directory.inode = parent_inode;
							lseek(device, overall_offset, SEEK_SET);
							write(device, &directory, sizeof(struct ext2_dir_entry_2));
						}
						if (link_count_flag == 1)
							local_link_count[parent_inode-1]++;
					}	
//					if (((inode_bitmap[((directory.inode-1)/8)] >> ((directory.inode-1)%8)) & 0x1) == 0x1)					

						local_inode_bitmap[((directory.inode-1)/8)]=local_inode_bitmap[((directory.inode-1)/8)] | (0x1<<((directory.inode-1)%8));
					
//					printf("\n%10d\t%d\t%d\t%d", directory.inode, directory.rec_len, directory.name_len, directory.file_type);
//					printf("\n%10d\t%d", directory.inode, directory.file_type);		
//					printf("\t%20s", directory.name);

					if (directory.file_type != 2 & link_count_flag == 1)
					{
						struct ext2_inode temp_inode;
						lseek(device, offset_inode(directory.inode, group_descriptor, super), SEEK_SET);
						read(device, &temp_inode, sizeof(struct ext2_inode));
						link_count[directory.inode-1] = temp_inode.i_links_count;
						local_link_count[directory.inode-1]++;

					}
					if ((strcmp(directory.name, ".") != 0) & (strcmp(directory.name, "..") != 0) & (directory.file_type == 2 | directory.file_type == 1))
					{
						child_inode = directory.inode;
						parent_inode = temp;
						links_count++;
						if (link_count_flag == 1 & directory.file_type == 2)
							local_link_count[directory.inode-1]++;
						get_inode(device, parent_inode, child_inode, 2, group_descriptor, super, inode_bitmap, local_inode_bitmap, block_bitmap, local_block_bitmap, link_count, local_link_count);
					}

					overall_offset += directory.rec_len;
					lseek(device, overall_offset, SEEK_SET);
					read(device, &directory, sizeof(struct ext2_dir_entry_2));
				}while ((strcmp(directory.name, ".") != 0) & (directory.inode != 0) & (directory.file_type == 1 | directory.file_type == 2 | directory.file_type == 7));
				
/*			if (inode.i_links_count != link_count)
			{
				printf("\ninode %d actual %d observed %d", temp, inode.i_links_count, link_count);	
				inode.i_links_count = link_count;
				lseek(device, offset_inode(temp, group_descriptor, super), SEEK_SET);
				write(device, &inode, sizeof(struct ext2_inode));
			}
*/			}
			
		}
		if ((inode.i_mode & 0xF000) != 0x4000 & block_count_flag == 1)
		{
			if (inode.i_block[i] != 0)
			{	
				counter++;
//				printf("\nBLOCK %d", inode.i_block[i]);				
				local_block_bitmap[inode.i_block[i]] = 1;
			}
		}
		i++;
	}
	
}

void bitmap_comparison(int device, unsigned char *local_inode_bitmap, unsigned char *inode_bitmap, struct ext2_group_desc **group_descriptor, struct ext2_super_block **super, unsigned char *block_bitmap, unsigned char *local_block_bitmap, unsigned char *link_count, unsigned char *local_link_count)
{
	int inode_bit, i, j, temp;
	unsigned char bitmap_bit, bitmap_byte;
	struct ext2_inode inode;
	struct ext2_dir_entry_2 directory, temp_directory;	
	__u32 overall_offset;
	__u32 temp_offset = 0;
	for (i=11; i<super[0]->s_inodes_count; i++)
	{
		overall_offset = partition_offset;		
		if (((local_inode_bitmap[(i)/8] >> ((i) % 8)) & 0x1) != ((inode_bitmap[(i)/8] >> ((i) % 8)) & 0x1))
		{				
			inode_bit = i+1;
			lseek(device, offset_inode(inode_bit, group_descriptor, super), SEEK_SET);
			read(device, &inode, sizeof(struct ext2_inode));
			directory.inode = inode_bit;
			if ((inode.i_mode & 0xF000) == 0x4000)
				directory.file_type = 2;
			else
				directory.file_type = 1;
			directory.rec_len = 0;
		
			directory.name_len = 0;
			while (inode_bit > 9)
			{
				inode_bit /= 10;
				directory.name_len++;
			}
			directory.name_len++;
			inode_bit = directory.inode;
			j=0;
			while (j < directory.name_len)
			{
				directory.name[directory.name_len-1-j] = (inode_bit % 10) + '0';
				inode_bit /= 10;	
				j++;
			}
			directory.name[directory.name_len] = '\0';

			do
			{
			lseek(device, lost_found_offset + temp_offset, SEEK_SET);
			read(device, &temp_directory, sizeof(struct ext2_dir_entry_2));
			temp_offset += temp_directory.rec_len;
			}while(temp_offset < 1024);
			temp_offset -= temp_directory.rec_len;
			temp_directory.rec_len = 20;

			lseek(device, lost_found_offset + temp_offset, SEEK_SET);
			write(device, &temp_directory, sizeof(struct ext2_dir_entry_2));
			temp_offset += temp_directory.rec_len;		
			directory.rec_len = 1024 - temp_offset;
			lseek(device, lost_found_offset + temp_offset, SEEK_SET);			
			write(device, &directory, sizeof(struct ext2_dir_entry_2));

			get_inode(device, 2, 2, 1, group_descriptor, super, inode_bitmap, local_inode_bitmap, block_bitmap, local_block_bitmap, link_count, local_link_count);						
		}
	}
}

void link_count_check(int device, unsigned char *link_count, unsigned char *local_link_count, struct ext2_group_desc **group_descriptor, struct ext2_super_block **super)
{
	struct ext2_inode inode;
	int i;	
		
	for (i=0; i<super[0]->s_inodes_count; i++)
	{
		if (link_count[i] != local_link_count[i])
		{	
			printf("\nInode %d: actual count %d observed count %d", i+1, link_count[i], local_link_count[i]);
			lseek(device, offset_inode(i+1, group_descriptor, super), SEEK_SET);
			read(device, &inode, sizeof(struct ext2_inode));
			inode.i_links_count = local_link_count[i];
			lseek(device, offset_inode(i+1, group_descriptor, super), SEEK_SET);
			write(device, &inode, sizeof(struct ext2_inode));			
		} 
	}
	link_count_flag = 0;
}

void block_comparison(int device, unsigned char *local_block_bitmap, unsigned char *block_bitmap, struct ext2_group_desc **group_descriptor, struct ext2_super_block **super)
{
	int i, j, no_of_used_blocks = 0;
	unsigned char buf[super[0]->s_log_block_size];
//	printf("\nlocal bitmap");
	for (i=1; i<super[0]->s_blocks_count; i++)
	{
//		printf("\n%d. %d", i, local_block_bitmap[i]);
		if (local_block_bitmap[i] != 0)
			no_of_used_blocks++;
		if ((local_block_bitmap[i] != 0) & ((block_bitmap[(i-1)/8] >> ((i-1) % 8)) & 0x1) == 0)
		{		

			printf("\nBLOCK %d", i);
			block_bitmap[((i-1)/8)]=block_bitmap[((i-1)/8)] | (0x1<<((i-1)%8));
		}
	}
	printf("\nObserved count: %d", no_of_used_blocks);
	for (i=0; i<3; i++)
	{ 
		lseek(device, partition_offset + (group_descriptor[i]->bg_block_bitmap * (1024 << super[0]->s_log_block_size)), SEEK_SET);
		write(device, ((super[0]->s_blocks_per_group*i)/8)+block_bitmap, 1024);
	}
	printf("\nCounter value %d", counter);				
		
}

void allocate_partition(struct partition_custom **temp2, int *extended_sector_no)
{
	unsigned char buf[sector_size_bytes];
	struct partition_custom *p = *temp2;
	int sector_bias = 446, i=1;	
	int first_ebr = (p->temp).start_sect;	
	int local_no = *extended_sector_no;
	int start_sector = first_ebr;

	struct partition_custom *inter;
	while (1)
	{			
		read_sectors(start_sector, 1, buf);
		struct partition_custom *inter = (struct partition_custom*)malloc(sizeof(struct partition_custom));
		memcpy (&(inter->temp), buf+sector_bias, 16);
		(inter->temp).start_sect += start_sector;
		inter->part_no = local_no++;
		p->next = inter;
		p = p->next;
		struct partition_custom *temporary = (struct partition_custom*)malloc(sizeof(struct partition_custom));;
		memcpy(&(temporary->temp), buf+sector_bias+16, 16);
		if ((temporary->temp).nr_sects == 0)
			break;
		else
			start_sector = first_ebr+(temporary->temp).start_sect;
	}
		*temp2 = p;
		*extended_sector_no = local_no;
}

int main (int argc, char **argv)
{

	unsigned char buf[sector_size_bytes];
	int the_sector;
	if ((device = open(argv[4], O_RDWR)) == -1)
	{
		perror("Could not open device file");
		exit(-1);
	}
	the_sector = atoi(argv[0]);
	read_sectors(the_sector, 1, buf);
	struct partition_custom *p, *head;
	head = malloc(sizeof(struct partition_custom));
	int i, local_flag = -1, args = -1, extended_sector_no = 5;
	int j,k, flag, sector_bias = 446, length, first_inode = 0;
	i = 2;
	k = 1;

//Start of part 1:
	head->part_no = 1;
	k = head->part_no;
	memcpy(&(head->temp), buf+sector_bias, 16);
	p = head;	
	while (i <= 4)
	{
		sector_bias += 16;
		p->next = malloc(sizeof(struct partition_custom));
		memcpy(&((p->next)->temp), buf+sector_bias, 16);

		p = p->next;
		p->part_no = ++k;
		if ((p->temp).sys_ind == 0x05)
		{
			allocate_partition(&p, &extended_sector_no);
		}
		++i;
	}

	p = head;
	while (p != NULL)
	{
		if (p->part_no == atoi(argv[2]))
		{
			if (strcmp(argv[1], "-p") == 0)
				printf("0x%02X %d %d\n", (p->temp).sys_ind, (p->temp).start_sect, (p->temp).nr_sects);
			args = p->part_no;
			break;
		}
		p = p->next;
	}
	if (args == -1 & strcmp(argv[1], "-p") == 0)
		printf("-1\n");
	
//Start of part 2:
	if (strcmp(argv[1], "-f") == 0)
	{
		if (atoi(argv[2]) == 0)
			local_flag = 0;
		p = head;
		while (p != NULL)
		{
			if (p->part_no == atoi(argv[2]) & (p->temp).sys_ind == 0x83)
			{
				local_flag = p->part_no;
				break;
			}
			p = p->next;
		}
		if (local_flag == -1)
		{
			close(device);
			return 0;
		}
		if (local_flag == 0)
			p = head;
		while (p != NULL)
		{
			if ((p->temp).sys_ind == 0x83)
			{
				partition_offset = (p->temp).start_sect * sector_size_bytes;
				lost_found_offset = 0;
				struct ext2_super_block *super_block[10];
				struct ext2_group_desc *group_descriptor[10];
				int span_of_inodes = 0;		
				get_super_and_group(device, super_block, group_descriptor);
				unsigned char inode_bitmap[(super_block[0]->s_inodes_count/8)+1];
				unsigned char local_inode_bitmap[(super_block[0]->s_inodes_count/8)+1];
				unsigned char block_bitmap[(super_block[0]->s_blocks_per_group*3)];
				unsigned char local_block_bitmap[(super_block[0]->s_blocks_per_group*3)];
				unsigned char link_count[super_block[0]->s_inodes_count];
				unsigned char local_link_count[super_block[0]->s_inodes_count];

				for (i=0; i<(super_block[0]->s_inodes_count/8)+1; i++)
				{
					local_inode_bitmap[i] = 0;
					inode_bitmap[i] = 0;
				}
				for (i=0; i<(super_block[0]->s_blocks_per_group*3); i++)
				{
					local_block_bitmap[i] = 0;
					block_bitmap[i] = 0;
				}
				for (i=0; i<super_block[0]->s_inodes_count; i++)
				{
					local_link_count[i] = 0;
					link_count[i] = 0;
				}
//				if ((1024 << super_block[0]->s_log_block_size) == 1024)
					local_block_bitmap[1] = 1;
//				else
//					local_block_bitmap[0] = 1;

				local_block_bitmap[2] = 1;
				local_block_bitmap[group_descriptor[0]->bg_block_bitmap] = 1;
				local_block_bitmap[group_descriptor[0]->bg_inode_bitmap] = 1;
				local_block_bitmap[group_descriptor[0]->bg_inode_table] = 1;
				span_of_inodes = (super_block[0]->s_inodes_per_group * super_block[0]->s_inode_size)/(1024 << super_block[0]->s_log_block_size);
				for (i=1; i<=span_of_inodes; i++)
					local_block_bitmap[group_descriptor[0]->bg_inode_table + i] = 1;

				for (i=1; i<groups-1; i++)
				{
					local_block_bitmap[group_descriptor[i]->bg_block_bitmap] = 1;
					local_block_bitmap[group_descriptor[i]->bg_inode_bitmap] = 1;
					local_block_bitmap[group_descriptor[i]->bg_inode_table] = 1;		
					span_of_inodes = (super_block[i]->s_inodes_per_group * super_block[i]->s_inode_size)/(1024 << super_block[i]->s_log_block_size);
					for (j=1; j<=span_of_inodes; j++)
						local_block_bitmap[group_descriptor[i]->bg_inode_table + j] = 1;
				}

				for (i=0; i<3; i++)
				{ 
					lseek(device, partition_offset + (group_descriptor[i]->bg_inode_bitmap * (1024 << super_block[0]->s_log_block_size)), SEEK_SET);
					read(device, ((super_block[0]->s_inodes_per_group*i)/8)+inode_bitmap, sizeof(inode_bitmap));
					lseek(device, partition_offset + (group_descriptor[i]->bg_block_bitmap * (1024 << super_block[0]->s_log_block_size)), SEEK_SET);
					read(device, ((super_block[0]->s_blocks_per_group*i)/8)+block_bitmap, 1024);
				}
				

				get_inode(device, 2, 2, 2, group_descriptor, super_block, inode_bitmap, local_inode_bitmap, block_bitmap, local_block_bitmap, link_count, local_link_count);
				
				bitmap_comparison(device, local_inode_bitmap, inode_bitmap, group_descriptor, super_block, block_bitmap, local_block_bitmap, link_count, local_link_count);
				
				link_count_flag = 1;
				get_inode(device, 2, 2, 2, group_descriptor, super_block, inode_bitmap, local_inode_bitmap, block_bitmap, local_block_bitmap, link_count, local_link_count);
				link_count_check(device, link_count, local_link_count, group_descriptor, super_block);

				link_count_flag = 0;				
				block_count_flag = 1;
				get_inode(device, 2, 2, 2, group_descriptor, super_block, inode_bitmap, local_inode_bitmap, block_bitmap, local_block_bitmap, link_count, local_link_count);
				block_comparison(device, local_block_bitmap, block_bitmap, group_descriptor, super_block);

			}
			if (local_flag == 0)
				p = p->next;
			else
				break;
		}
		printf("\n");
	}

close(device);
	return 0;
}
