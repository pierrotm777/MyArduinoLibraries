/*
 English: by RC Navy (2023)
 =======
 <H_BridgeCmd>: a library to command an H-Bridge having 2 command pins (Cmd1 & Cmd2).
 http://p.loussouarn.free.fr
 V1.0: (04/11/2023) initial release

 Francais: par RC Navy (2023)
 ========
 <H_BridgeCmd>: une bibliotheque pour commander un pont en H ayant 2 broches de commande (Cmd1 & Cmd2).
 http://p.loussouarn.free.fr
 V1.0: (04/11/2023) release initiale
                                               +6V
                                               ---
                                                |
                                      .---------o---------.
                                      |                   |
                                      |                   |
                               .------/ T1             T3 /------.
                    .-----.    |      |                   |      |
 Cmd1 >-----o-------+      \   |      |     .-------.     |      |
            |       | AND   )--o      o-----| MOTOR |-----o      o--.
 Cmd2 >--o-----|>o--+      /   |      |     '-------'     |      |  |
         |  |       '-----'    '------|---------.         |      |  |
         |  |                      T2 /-------.  '--------/ T4   |  |
         |  |                         |        '----------|------'  |
         |  |                         |                   |         |
         |  |                         '---------o---------'         |
         |  |                                   |                   |
         |  |                                  ---                  |
         |  |       .-----.                     GND                 |
         |  '--|>o--+      \                                        |
         |          | AND   )---------------------------------------'
         '----------+      /
                    '-----'
*/
#ifndef H_BRIDGE_CMD_H
#define H_BRIDGE_CMD_H

#define H_BRIDGE_CMD_VERSION    1
#define H_BRIDGE_CMD_REVISION   0

#include "Arduino.h"
#include <inttypes.h>

typedef struct{
    uint8_t     Cmd1_Pin;
    uint8_t     Cmd2_Pin;
    uint8_t
                Inv       :1,
                Reserved  :7;
}H_BridgeSt_t;

class H_BridgeCmd
{
  public:
    H_BridgeCmd(uint8_t Cmd1_Pin, uint8_t Cmd2_Pin, uint8_t InvCmd = 0);
    void              command(uint8_t Dir, uint8_t PwmVal);
    H_BridgeSt_t      _Cmd;
};

#endif
