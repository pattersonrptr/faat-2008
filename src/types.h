#ifndef TYPES_H
#define TYPES_H

#include <time.h>

/* =========================================================
 *  Limites máximos (arrays estáticos — sem malloc excessivo)
 * ========================================================= */
#define MAX_ROOMS        200
#define MAX_GUESTS      5000
#define MAX_RESERVATIONS 2000
#define MAX_SERVICES    10000
#define MAX_STAFF         100
#define MAX_ROOMS_PER_RES  10   /* quartos por reserva */
#define MAX_MAINT_PER_ROOM 20   /* histórico de manutenções por quarto */
#define MAX_LOST_ITEMS    500

/* =========================================================
 *  Enums — Quartos
 * ========================================================= */
typedef enum {
    ROOM_SOLTEIRO    = 0,
    ROOM_DUPLO       = 1,
    ROOM_SUITE       = 2,
    ROOM_PRESIDENCIAL = 3
} RoomType;

typedef enum {
    ROOM_LIVRE       = 0,
    ROOM_OCUPADO     = 1,
    ROOM_RESERVADO   = 2,
    ROOM_MANUTENCAO  = 3,
    ROOM_BLOQUEADO   = 4
} RoomStatus;

/* Bitfield de características do quarto */
#define FEAT_AC          (1 << 0)   /* ar-condicionado */
#define FEAT_FRIGOBAR    (1 << 1)
#define FEAT_BANHEIRA    (1 << 2)
#define FEAT_VARANDA     (1 << 3)
#define FEAT_ACESSIVEL   (1 << 4)

typedef enum {
    CLEAN_SUJO      = 0,
    CLEAN_EM_LIMPEZA = 1,
    CLEAN_LIMPO     = 2,
    CLEAN_INSPECIONADO = 3
} CleanStatus;

/* =========================================================
 *  Enums — Hóspedes
 * ========================================================= */
typedef enum {
    GUEST_REGULAR    = 0,
    GUEST_VIP        = 1,
    GUEST_CORPORATIVO = 2,
    GUEST_GRUPO      = 3
} GuestType;

typedef enum {
    FIDELITY_BRONZE   = 0,
    FIDELITY_PRATA    = 1,
    FIDELITY_OURO     = 2,
    FIDELITY_DIAMANTE = 3
} FidelityLevel;

/* =========================================================
 *  Enums — Reservas
 * ========================================================= */
typedef enum {
    RES_PENDENTE      = 0,
    RES_CONFIRMADA    = 1,
    RES_CHECKIN_FEITO = 2,
    RES_CHECKOUT_FEITO = 3,
    RES_CANCELADA     = 4,
    RES_NO_SHOW       = 5
} ReservationStatus;

/* =========================================================
 *  Enums — Serviços
 * ========================================================= */
typedef enum {
    SVC_FRIGOBAR      = 0,
    SVC_ROOM_SERVICE  = 1,
    SVC_LAVANDERIA    = 2,
    SVC_ESTACIONAMENTO = 3,
    SVC_TELEFONIA     = 4,
    SVC_OUTRO         = 5
} ServiceCategory;

typedef enum {
    SVC_STATUS_PENDENTE   = 0,
    SVC_STATUS_ENTREGUE   = 1,
    SVC_STATUS_CANCELADO  = 2
} ServiceStatus;

/* =========================================================
 *  Enums — Financeiro
 * ========================================================= */
typedef enum {
    PAY_DINHEIRO     = 0,
    PAY_CREDITO      = 1,
    PAY_DEBITO       = 2,
    PAY_PIX          = 3,
    PAY_TRANSFERENCIA = 4,
    PAY_FATURADO     = 5   /* empresa — contas a receber */
} PaymentMethod;

/* =========================================================
 *  Enums — Funcionários
 * ========================================================= */
typedef enum {
    ROLE_RECEPCIONISTA = 0,
    ROLE_GERENTE       = 1,
    ROLE_CAMAREIRO     = 2,
    ROLE_MANUTENCAO    = 3,
    ROLE_SEGURANCA     = 4,
    ROLE_COZINHEIRO    = 5,
    ROLE_GARCOM        = 6
} StaffRole;

/* Níveis de acesso (1 = básico, 5 = admin) */
#define ACCESS_BASICO    1
#define ACCESS_OPERADOR  2
#define ACCESS_SUPERVISOR 3
#define ACCESS_GERENTE   4
#define ACCESS_ADMIN     5

/* =========================================================
 *  Structs
 * ========================================================= */

typedef struct {
    int    id;
    char   description[100];
    double cost;
    char   technician[80];
    time_t date;
} Maintenance;

typedef struct {
    int         id;
    char        number[8];          /* ex: "101", "304A" */
    int         floor;
    RoomType    type;
    int         capacity;
    double      daily_rate;
    RoomStatus  status;
    CleanStatus clean_status;
    int         features;           /* bitfield FEAT_* */
    int         assigned_staff_id;  /* camareiro responsável (0 = nenhum) */
    Maintenance maintenance_history[MAX_MAINT_PER_ROOM];
    int         maintenance_count;
} Room;

typedef struct {
    int          id;
    char         full_name[100];
    char         document[20];      /* CPF ou passaporte */
    char         birth_date[11];    /* "DD/MM/AAAA" */
    char         nationality[40];
    char         gender;            /* 'M', 'F', 'O' */
    char         phone[20];
    char         email[80];
    char         address[200];
    GuestType    type;
    FidelityLevel fidelity_level;
    int          fidelity_points;
    int          blacklisted;
    char         blacklist_reason[200];
    int          total_stays;
    double       total_spent;
    time_t       last_visit;        /* 0 se nunca hospedado */
} Guest;

typedef struct {
    int               id;
    char              code[12];             /* código de confirmação alfanumérico */
    int               guest_id;
    int               room_ids[MAX_ROOMS_PER_RES];
    int               room_count;
    time_t            check_in_date;
    time_t            check_out_date;
    time_t            actual_check_in;      /* 0 se ainda não fez */
    time_t            actual_check_out;     /* 0 se ainda não fez */
    int               adults;
    int               children;
    ReservationStatus status;
    double            base_value;           /* soma das diárias */
    double            services_total;       /* consumos lançados */
    double            discount;
    double            total_value;
    PaymentMethod     payment_method;
    int               payment_installments; /* parcelas (cartão crédito) */
    int               created_by;           /* staff_id */
    time_t            created_at;
    char              notes[300];
} Reservation;

typedef struct {
    int             id;
    int             reservation_id;
    ServiceCategory category;
    char            description[200];
    double          unit_price;
    int             quantity;
    ServiceStatus   status;
    time_t          registered_at;
    int             registered_by;  /* staff_id */
} ServiceItem;

typedef struct {
    int       id;
    char      full_name[80];
    char      document[20];
    StaffRole role;
    char      department[40];
    double    salary;
    char      hire_date[11];        /* "DD/MM/AAAA" */
    int       shift;                /* 0=manhã, 1=tarde, 2=noite */
    int       access_level;
    char      username[30];
    char      password_hash[20];    /* hash djb2 em hex */
    int       active;
} Staff;

typedef struct {
    int    id;
    int    room_id;
    char   item_description[150];
    char   finder_notes[150];
    time_t found_date;
    int    returned;                /* 0=guardado, 1=devolvido, 2=descartado */
} LostItem;

typedef struct {
    char   hotel_name[100];
    char   address[200];
    char   cnpj[20];
    int    stars;                   /* 1 a 5 */
    char   checkin_time[6];         /* "HH:MM" */
    char   checkout_time[6];
    double tourist_tax;             /* taxa turística por diária */
    double service_fee_pct;         /* % sobre serviços */
    double late_checkout_fee;       /* valor fixo ou % da diária */
    double early_checkin_fee;
    int    cancel_free_days;        /* dias antes p/ cancelamento sem custo */
    double cancel_partial_pct;      /* % de multa no cancelamento parcial */
    /* Programa de fidelidade */
    double points_per_real;         /* pontos ganhos por R$1,00 gasto */
    double multiplier_prata;
    double multiplier_ouro;
    double multiplier_diamante;
} HotelConfig;

#endif /* TYPES_H */
