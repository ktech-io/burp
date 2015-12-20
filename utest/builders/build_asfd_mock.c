#include "../../src/burp.h"
#include "../../src/alloc.h"
#include "../../src/asfd.h"
#include "../../src/cmd.h"
#include "../../src/iobuf.h"
#include "build_asfd_mock.h"
#include <check.h>

static void ioevent_list_init(struct ioevent_list *list)
{
	memset(list, 0, sizeof(*list));
}

static void ioevent_list_grow(struct ioevent_list *list)
{
	list->size++;
	list->ioevent=(struct ioevent *)
		realloc_w(list->ioevent,
			list->size*sizeof(struct ioevent), __func__);
	fail_unless(list->ioevent!=NULL);
}

static int mock_asfd_read(struct asfd *asfd)
{
	struct ioevent *r;
	struct ioevent_list *reads=(struct ioevent_list *)asfd->data1;

printf("r %s %d %d\n", asfd->desc, reads->cursor, reads->size);
	fail_unless(reads->cursor<reads->size);

	r=&reads->ioevent[reads->cursor];
	if(r->no_op)
	{
		r->no_op--;
		if(!r->no_op)
			reads->cursor++;
		return r->ret;
	}
printf("r %s - %c:%s\n", asfd->desc, r->iobuf.cmd, r->iobuf.buf);
	iobuf_move(asfd->rbuf, &r->iobuf);
	reads->cursor++;
	return r->ret;
}

//#include "../../src/protocol2/blk.h"
//#include "../../src/hexmap.h"

static int asfd_assert_write(struct asfd *asfd, struct iobuf *wbuf)
{
	struct ioevent *w;
	struct iobuf *expected;
	struct ioevent_list *writes=(struct ioevent_list *)asfd->data2;
printf("w %s %d %d\n", asfd->desc, writes->cursor, writes->size);
	fail_unless(writes->cursor<writes->size);
	w=&writes->ioevent[writes->cursor++];
	expected=&w->iobuf;
/*
if(wbuf->cmd==CMD_SIG)
{
	struct blk blk;
                        blk_set_from_iobuf_sig(&blk, wbuf);
                        printf("%016"PRIX64" %s\n",
                                blk.fingerprint,
                                bytes_to_md5str(blk.md5sum));

}
*/
printf("%d\n", wbuf->len);
printf("w %s - %c:%s %c:%s\n", asfd->desc, wbuf->cmd, wbuf->buf,
		expected->cmd, expected->buf);
	fail_unless(wbuf->len==expected->len);
	fail_unless(wbuf->cmd==expected->cmd);
	fail_unless(!memcmp(expected->buf, wbuf->buf, wbuf->len));
	iobuf_free_content(expected);
	return w->ret;
}

static int mock_asfd_assert_write_str(struct asfd *asfd,
	enum cmd wcmd, const char *wsrc)
{
	struct iobuf wbuf;
	iobuf_set(&wbuf, wcmd, (char *)wsrc, strlen(wsrc));
	return asfd_assert_write(asfd, &wbuf);
}

static enum append_ret mock_asfd_assert_append_all_to_write_buffer(
	struct asfd *asfd, struct iobuf *wbuf)
{
	enum append_ret ret;
	ret=(enum append_ret)asfd_assert_write(asfd, wbuf);
	wbuf->len=0;
	return ret;
}

static int mock_parse_readbuf(struct asfd *asfd)
{
	return 0;
}

struct asfd *asfd_mock_setup(struct ioevent_list *user_reads,
	struct ioevent_list *user_writes)
{
	struct asfd *asfd=NULL;
	fail_unless((asfd=asfd_alloc())!=NULL);
	fail_unless((asfd->rbuf=iobuf_alloc())!=NULL);
	asfd->read=mock_asfd_read;
	asfd->write=asfd_assert_write;
	asfd->write_str=mock_asfd_assert_write_str;
	asfd->append_all_to_write_buffer=
		mock_asfd_assert_append_all_to_write_buffer;
	asfd->parse_readbuf=mock_parse_readbuf;
	ioevent_list_init(user_reads);
	ioevent_list_init(user_writes);
	asfd->data1=(void *)user_reads;
	asfd->data2=(void *)user_writes;
	return asfd;
};

void asfd_mock_teardown(struct ioevent_list *user_reads,
	struct ioevent_list *user_writes)
{
	free_v((void **)&user_reads->ioevent);
	free_v((void **)&user_writes->ioevent);
}

static void add_to_ioevent(struct ioevent_list *ioevent_list,
	int *i, int ret, enum cmd cmd, void *data, int dlen)
{
	struct ioevent *ioevent;
	ioevent_list_grow(ioevent_list);
	ioevent=ioevent_list->ioevent;
	ioevent[*i].ret=ret;
	ioevent[*i].no_op=0;
	ioevent[*i].iobuf.cmd=cmd;
	ioevent[*i].iobuf.len=dlen;
	ioevent[*i].iobuf.buf=NULL;
	if(dlen)
	{
		fail_unless((ioevent[*i].iobuf.buf=
			(char *)malloc_w(dlen+1, __func__))!=NULL);
		fail_unless(memcpy(ioevent[*i].iobuf.buf,
			data, dlen+1)!=NULL);
	}
	(*i)++;
}

static void add_no_op(struct ioevent_list *ioevent_list, int *i, int count)
{
	struct ioevent *ioevent;
	ioevent_list_grow(ioevent_list);
	ioevent=ioevent_list->ioevent;
	ioevent[*i].ret=0;
	ioevent[*i].no_op=count;
	(*i)++;
}

void asfd_mock_read(struct asfd *asfd,
	int *r, int ret, enum cmd cmd, const char *str)
{
	struct ioevent_list *reads=(struct ioevent_list *)asfd->data1;
	add_to_ioevent(reads, r, ret, cmd,
		(void *)str, str?strlen(str):0);
}

void asfd_assert_write(struct asfd *asfd,
	int *w, int ret, enum cmd cmd, const char *str)
{
	struct ioevent_list *writes=(struct ioevent_list *)asfd->data2;
	add_to_ioevent(writes, w, ret, cmd,
		(void *)str, str?strlen(str):0);
}

void asfd_mock_read_no_op(struct asfd *asfd, int *r, int count)
{
	struct ioevent_list *reads=(struct ioevent_list *)asfd->data1;
	add_no_op(reads, r, count);
}

void asfd_mock_read_iobuf(struct asfd *asfd,
	int *r, int ret, struct iobuf *iobuf)
{
	struct ioevent_list *reads=(struct ioevent_list *)asfd->data1;
	add_to_ioevent(reads, r, ret,
		iobuf->cmd, iobuf->buf, iobuf->len);
}

void asfd_assert_write_iobuf(struct asfd *asfd,
	int *w, int ret, struct iobuf *iobuf)
{
	struct ioevent_list *writes=(struct ioevent_list *)asfd->data2;
	add_to_ioevent(writes, w, ret,
		iobuf->cmd, iobuf->buf, iobuf->len);
}
