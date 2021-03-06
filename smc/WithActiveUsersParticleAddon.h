/***************************************************************************
 *   Copyright (C) 2006 by Manu   *
 *   manu@rustneversleeps   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef WITHACTIVEUSERSPARTICLEADDON_H
#define WITHACTIVEUSERSPARTICLEADDON_H

/**
It implements the code of a particle accounting for the users active at any time instant

	@author Manu <manu@rustneversleeps>
*/

#include <vector>
#include <types.h>

class WithActiveUsersParticleAddon{
protected:
    std::vector<std::vector<bool> > _activeUsers;
    uint _symbolVectorLength;
public:
    WithActiveUsersParticleAddon(uint symbolVectorLength,uint nTimeInstants);

    void setUserActivity(uint iUser,uint time,bool value) { _activeUsers[time][iUser] = value;}
    bool getUserActivity(uint iUser,uint time) const { return _activeUsers[time][iUser];}
    std::vector<bool> getActivityAtTime(uint time) const { return _activeUsers[time];}
    void setActivityAtTime(uint time,const std::vector<bool> &usersActivity) { _activeUsers[time] = usersActivity;}    

};

#endif
