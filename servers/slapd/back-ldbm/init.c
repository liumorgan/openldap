/* init.c - initialize ldbm backend */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-ldbm.h"

int
ldbm_back_initialize(
    BackendInfo	*bi
)
{
	bi->bi_open = ldbm_back_open;
	bi->bi_config = NULL;
	bi->bi_close = ldbm_back_close;
	bi->bi_destroy = ldbm_back_destroy;

	bi->bi_db_init = ldbm_back_db_init;
	bi->bi_db_config = ldbm_back_db_config;
	bi->bi_db_open = ldbm_back_db_open;
	bi->bi_db_close = ldbm_back_db_close;
	bi->bi_db_destroy = ldbm_back_db_destroy;

	bi->bi_op_bind = ldbm_back_bind;
	bi->bi_op_unbind = ldbm_back_unbind;
	bi->bi_op_search = ldbm_back_search;
	bi->bi_op_compare = ldbm_back_compare;
	bi->bi_op_modify = ldbm_back_modify;
	bi->bi_op_modrdn = ldbm_back_modrdn;
	bi->bi_op_add = ldbm_back_add;
	bi->bi_op_delete = ldbm_back_delete;
	bi->bi_op_abandon = ldbm_back_abandon;

#ifdef SLAPD_ACLGROUPS
	bi->bi_acl_group = ldbm_back_group;
#endif

	return 0;
}

int
ldbm_back_destroy(
    BackendInfo	*bi
)
{
	return 0;
}

int
ldbm_back_open(
    BackendInfo	*bi
)
{
	int rc;

	/* initialize the underlying database system */
	rc = ldbm_initialize();

	return rc;
}

int
ldbm_back_close(
    BackendInfo	*bi
)
{
	/* initialize the underlying database system */
	ldbm_shutdown();

	return 0;
}

int
ldbm_back_db_init(
    Backend	*be
)
{
	struct ldbminfo	*li;
	char		*argv[ 4 ];
	int		i;

	/* allocate backend-database-specific stuff */
	li = (struct ldbminfo *) ch_calloc( 1, sizeof(struct ldbminfo) );

	/* arrange to read nextid later (on first request for it) */
	li->li_nextid = NOID;

#if SLAPD_NEXTID_CHUNK > 1
	li->li_nextid_wrote = NOID;
#endif

	/* default cache size */
	li->li_cache.c_maxsize = DEFAULT_CACHE_SIZE;

	/* default database cache size */
	li->li_dbcachesize = DEFAULT_DBCACHE_SIZE;

	/* default cache mode is sync on write */
	li->li_dbcachewsync = 1;

	/* default file creation mode */
	li->li_mode = DEFAULT_MODE;

	/* default database directory */
	li->li_directory = DEFAULT_DB_DIRECTORY;

	/* always index dn, id2children, objectclass (used in some searches) */
	argv[ 0 ] = "dn";
	argv[ 1 ] = "dn";
	argv[ 2 ] = NULL;
	attr_syntax_config( "ldbm dn initialization", 0, 2, argv );
	argv[ 0 ] = "dn";
	argv[ 1 ] = "sub";
	argv[ 2 ] = "eq";
	argv[ 3 ] = NULL;
	attr_index_config( li, "ldbm dn initialization", 0, 3, argv, 1 );
	argv[ 0 ] = "id2children";
	argv[ 1 ] = "eq";
	argv[ 2 ] = NULL;
	attr_index_config( li, "ldbm id2children initialization", 0, 2, argv,
	    1 );
	argv[ 0 ] = "objectclass";
	argv[ 1 ] = ch_strdup( "pres,eq" );
	argv[ 2 ] = NULL;
	attr_index_config( li, "ldbm objectclass initialization", 0, 2, argv,
	    1 );
	free( argv[ 1 ] );

	/* initialize various mutex locks & condition variables */
	ldap_pvt_thread_mutex_init( &li->li_root_mutex );
	ldap_pvt_thread_mutex_init( &li->li_add_mutex );
	ldap_pvt_thread_mutex_init( &li->li_cache.c_mutex );
	ldap_pvt_thread_mutex_init( &li->li_nextid_mutex );
	ldap_pvt_thread_mutex_init( &li->li_dbcache_mutex );
	ldap_pvt_thread_cond_init( &li->li_dbcache_cv );

	be->be_private = li;

	return 0;
}

int
ldbm_back_db_open(
    BackendDB	*be
)
{
	return 0;
}

int
ldbm_back_db_destroy(
    BackendDB	*be
)
{
	/* should free/destroy every in be_private */
	free( be->be_private );
	be->be_private = NULL;
	return 0;
}
