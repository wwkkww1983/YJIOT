package com.yjyz.iot.ota.dao;

import java.util.List;

import com.yjyz.iot.ota.entity.OTAFileInfo;

public interface OTAFileInfoMapper {
	int deleteByPrimaryKey(Integer id);

	int insert(OTAFileInfo record);

	int insertSelective(OTAFileInfo record);

	OTAFileInfo selectByPrimaryKey(Integer id);

	OTAFileInfo selectByOTACheck(OTAFileInfo otaCheck);

	int updateByPrimaryKeySelective(OTAFileInfo record);

	int updateByPrimaryKey(OTAFileInfo record);

	List<OTAFileInfo> selectByProductId(String productId);

	int getOTAFileInofMaxId();

}