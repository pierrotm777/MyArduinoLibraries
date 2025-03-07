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
#include "H_BridgeCmd.h"

#define H_BRIDGE_PWM(PwmVal)    (_Cmd.Inv? (255 - (PwmVal)): (PwmVal))

H_BridgeCmd::H_BridgeCmd(uint8_t Cmd1_Pin, uint8_t Cmd2_Pin, uint8_t InvCmd /*= 0*/)
{
  _Cmd.Cmd1_Pin = Cmd1_Pin;
  _Cmd.Cmd2_Pin = Cmd2_Pin;
  _Cmd.Inv      = InvCmd;

  pinMode(_Cmd.Cmd1_Pin, OUTPUT);
  pinMode(_Cmd.Cmd2_Pin, OUTPUT);
  analogWrite(_Cmd.Cmd1_Pin, H_BRIDGE_PWM(0));
  analogWrite(_Cmd.Cmd2_Pin, H_BRIDGE_PWM(0));
}

void H_BridgeCmd::command(uint8_t Dir, uint8_t PwmVal)
{
  if(Dir)
  {
    /* REAR */
    analogWrite(_Cmd.Cmd1_Pin, H_BRIDGE_PWM(0));
    analogWrite(_Cmd.Cmd2_Pin, H_BRIDGE_PWM(PwmVal));
  }
  else
  {
    /* FORWARD */
    analogWrite(_Cmd.Cmd2_Pin, H_BRIDGE_PWM(0));
    analogWrite(_Cmd.Cmd1_Pin, H_BRIDGE_PWM(PwmVal));
  }
}
