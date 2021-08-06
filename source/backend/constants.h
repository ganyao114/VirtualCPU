//
// Created by swift on 2021/5/21.
//

#pragma once

#define REG_CTX     x0
#define REG_PT      x1
#define REG_STATUS  x2
#define REG_IP      x3
#define REG_IP_W    w3

#define OFFSET_PT           1048
#define OFFSET_CODE_CACHE   1032
#define OFFSET_NZCV         1096

#define OFFSET_X64_VREGS    184
#define OFFSET_X64_STATUS   440