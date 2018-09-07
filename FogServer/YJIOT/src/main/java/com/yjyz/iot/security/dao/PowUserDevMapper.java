package com.yjyz.iot.security.dao;

import java.util.List;

import com.yjyz.iot.security.entity.DeviceInfoQuery;
import com.yjyz.iot.security.entity.PowClientUserQuery;
import com.yjyz.iot.security.entity.PowUserDev;
import com.yjyz.iot.security.entity.PowUserDevKey;

public interface PowUserDevMapper {
	int deleteByPrimaryKey(PowUserDevKey key);

	int insert(PowUserDev record);

	int insertSelective(PowUserDev record);

	PowUserDev selectByPrimaryKey(PowUserDevKey key);

	int updateByPrimaryKeySelective(PowUserDev record);

	int updateByPrimaryKey(PowUserDev record);

	List<DeviceInfoQuery> selectUserDev(String clientId);

	List<PowClientUserQuery> selectDevUser(String deviceId);
}