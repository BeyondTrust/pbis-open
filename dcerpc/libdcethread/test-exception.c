#include <dce/dcethread.h>
#include <moonunit/interface.h>

static dcethread_exc dummy_e;
static dcethread_exc dummy2_e;

MU_FIXTURE_SETUP(exception)
{
    DCETHREAD_EXC_INIT(dummy_e);
    DCETHREAD_EXC_INIT(dummy2_e);
}

MU_TEST(exception, nothrow)
{
    volatile int reached_finally = 0;

    DCETHREAD_TRY
    {
	MU_ASSERT(!reached_finally);
    }
    DCETHREAD_CATCH(dummy_e)
    {
	MU_FAILURE("Reached catch block");
    }
    DCETHREAD_FINALLY
    {
	MU_ASSERT(!reached_finally);
	reached_finally = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(reached_finally);
}

MU_TEST(exception, throw_catch)
{
    volatile int reached_finally = 0;
    volatile int caught = 0;

    DCETHREAD_TRY
    {
	DCETHREAD_RAISE(dummy_e);
    }
    DCETHREAD_CATCH(dummy_e)
    {
	MU_ASSERT(!reached_finally);
	caught = 1;
    }
    DCETHREAD_FINALLY
    {
	reached_finally = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(caught);
    MU_ASSERT(reached_finally);
}

MU_TEST(exception, throw_catch_throw_catch)
{
    volatile int caught_inner = 0;
    volatile int caught_outer = 0;
    volatile int reached_inner_finally = 0;
    volatile int reached_outer_finally = 0;

    DCETHREAD_TRY
    {
	DCETHREAD_TRY
	{
	    MU_ASSERT(!caught_inner);
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    DCETHREAD_RAISE(dummy2_e);
	}
	DCETHREAD_CATCH(dummy2_e)
	{
	    MU_ASSERT(!caught_inner);
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    caught_inner = 1;
	    DCETHREAD_RAISE(dummy_e);
	}
	DCETHREAD_FINALLY
	{
	    MU_ASSERT(caught_inner);
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    reached_inner_finally = 1;
	}
	DCETHREAD_ENDTRY;
    }
    DCETHREAD_CATCH(dummy_e)
    {
	MU_ASSERT(caught_inner);
	MU_ASSERT(reached_inner_finally);
	MU_ASSERT(!caught_outer);
	MU_ASSERT(!reached_outer_finally);
	caught_outer = 1;
    }
    DCETHREAD_FINALLY
    {
	MU_ASSERT(caught_inner);
	MU_ASSERT(reached_inner_finally);
	MU_ASSERT(caught_outer);
	MU_ASSERT(!reached_outer_finally);
	reached_outer_finally = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(caught_inner);
    MU_ASSERT(reached_inner_finally);
    MU_ASSERT(caught_outer);
    MU_ASSERT(reached_outer_finally);
}

MU_TEST(exception, throw_catch_finally_throw_catch)
{
    volatile int caught_inner = 0;
    volatile int caught_outer = 0;
    volatile int reached_inner_finally = 0;
    volatile int reached_outer_finally = 0;

    DCETHREAD_TRY
    {
	DCETHREAD_TRY
	{
	    MU_ASSERT(!caught_inner);
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    DCETHREAD_RAISE(dummy2_e);
	}
	DCETHREAD_CATCH(dummy2_e)
	{
	    MU_ASSERT(!caught_inner);
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    caught_inner = 1;
	}
	DCETHREAD_FINALLY
	{
	    MU_ASSERT(caught_inner);
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    reached_inner_finally = 1;
	    DCETHREAD_RAISE(dummy_e);
	}
	DCETHREAD_ENDTRY;
    }
    DCETHREAD_CATCH(dummy_e)
    {
	MU_ASSERT(caught_inner);
	MU_ASSERT(reached_inner_finally);
	MU_ASSERT(!caught_outer);
	MU_ASSERT(!reached_outer_finally);
	caught_outer = 1;
    }
    DCETHREAD_FINALLY
    {
	MU_ASSERT(caught_inner);
	MU_ASSERT(reached_inner_finally);
	MU_ASSERT(caught_outer);
	MU_ASSERT(!reached_outer_finally);
	reached_outer_finally = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(caught_inner);
    MU_ASSERT(reached_inner_finally);
    MU_ASSERT(caught_outer);
    MU_ASSERT(reached_outer_finally);
}

MU_TEST(exception, throw_finally_throw_catch)
{
    volatile int caught_outer = 0;
    volatile int reached_inner_finally = 0;
    volatile int reached_outer_finally = 0;

    DCETHREAD_TRY
    {
	DCETHREAD_TRY
	{
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    DCETHREAD_RAISE(dummy2_e);
	}
	DCETHREAD_FINALLY
	{
	    MU_ASSERT(!reached_inner_finally);
	    MU_ASSERT(!caught_outer);
	    MU_ASSERT(!reached_outer_finally);
	    reached_inner_finally = 1;
	    DCETHREAD_RAISE(dummy_e);
	}
	DCETHREAD_ENDTRY;
    }
    DCETHREAD_CATCH(dummy_e)
    {
	MU_ASSERT(reached_inner_finally);
	MU_ASSERT(!caught_outer);
	MU_ASSERT(!reached_outer_finally);
	caught_outer = 1;
    }
    DCETHREAD_FINALLY
    {
	MU_ASSERT(reached_inner_finally);
	MU_ASSERT(caught_outer);
	MU_ASSERT(!reached_outer_finally);
	reached_outer_finally = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(reached_inner_finally);
    MU_ASSERT(caught_outer);
    MU_ASSERT(reached_outer_finally);
}

MU_TEST(exception, uncaught)
{
    MU_EXPECT(MU_STATUS_EXCEPTION);
    
    DCETHREAD_RAISE(dummy_e);
}
