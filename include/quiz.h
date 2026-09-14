/* quiz.h - the shared question-bank type for the quiz family. */
#ifndef GIC_QUIZ_H
#define GIC_QUIZ_H

#define QUIZ_TOPICS 16
#define QUIZ_LEVELS 4

typedef struct {
    unsigned char topic;      /* index into QUIZ_TOPIC            */
    unsigned char level;      /* 0 easy .. 3 expert               */
    const char   *q;
    const char   *a[4];
    unsigned char correct;    /* index into a[]                   */
} QItem;

extern const char *QUIZ_TOPIC[QUIZ_TOPICS];
extern const QItem QUIZ_BANK[];
extern const int   QUIZ_BANK_COUNT;
extern const QItem QUIZ_BANK2[];
extern const int   QUIZ_BANK2_COUNT;

#endif
