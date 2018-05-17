/*
 * muQuinet, an userspace TCP/IP network stack.
 * Copyright (C) 2018 rtdarwin
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef MUQUINETD_BASE_CMDRUNNER_H
#define MUQUINETD_BASE_CMDRUNNER_H

class CmdRunner
{
public:
    CmdRunner(const char* cmd, ...);
    // Rule of zero, Copyable, Moveable

    void run();

private:
    char _buf[128];
};

#endif // MUQUINETD_BASE_CMDRUNNER_H
