#ifdef _WIN32
#define RPC_FIELD_COUNT(x) ((x)->Count)
#define RPC_FIELD_BINDING_H(x) ((x)->BindingH)
#else
#define RPC_FIELD_COUNT(x) ((x)->count)
#define RPC_FIELD_BINDING_H(x) ((x)->binding_h)
#endif
