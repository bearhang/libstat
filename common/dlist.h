#ifndef __DLIST_H__
#define __DLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - ((size_t) &((type *)0)->member)); })
#endif

struct dlist_head {
    struct dlist_head *next;
    struct dlist_head *prev;
};

static inline int dlist_empty(struct dlist_head *list)
{
    return list->next == list;
}

static inline void dlist_init(struct dlist_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void _dlist_add(struct dlist_head *prev, struct dlist_head *next, struct dlist_head *new)
{
    new->next = next;
    new->prev = prev;
    next->prev = new;
    prev->next = new;
}

static inline void dlist_add_head(struct dlist_head *list, struct dlist_head *link)
{
    _dlist_add(list, list->next, link);
}

static inline void dlist_add_tail(struct dlist_head *list, struct dlist_head *link)
{
    _dlist_add(list->prev, list, link);
}

static inline void _dlist_del(struct dlist_head *prev, struct dlist_head *next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void dlist_del(struct dlist_head *link)
{
    _dlist_del(link->prev, link->next);
}

static inline void dlist_del_init(struct dlist_head *link)
{
    _dlist_del(link->prev, link->next);
    link->next = link;
    link->prev = link;
}

#define dlist_entry(link, type, member) container_of(link, type, member)

#define dlist_for_each(link, list)  \
    for (link = (list)->next; link != (list); link = link->next)

#define dlist_for_each_safe(link, safe, list)   \
    for (link = (list)->next, safe = link->next; link != (list); link = safe, safe = link->next)

#define dlist_for_each_entry(entry, list, member)   \
    for (entry = dlist_entry((list)->next, typeof(*entry), member); \
        &entry->member != (list);   \
        entry = dlist_entry(entry->member.next, typeof(*entry), member))

#define dlist_for_each_entry_safe(entry, safe, list, member)    \
    for (entry = dlist_entry((list)->next, typeof(*entry), member), \
        safe = dlist_entry(entry->member.next, typeof(*entry), member); \
        &entry->member != (list);   \
        entry = safe, safe = dlist_entry(entry->member.next, typeof(*entry), member); )

#ifdef __cplusplus
}
#endif

#endif
