package com.yjyz.iot.share.service.impl;

import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import javax.annotation.Resource;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import com.alibaba.fastjson.JSONObject;
import com.yjyz.iot.comm.ConstParm;
import com.yjyz.iot.device.dao.DevAccountMapper;
import com.yjyz.iot.device.dao.DevConsumeMapper;
import com.yjyz.iot.device.dao.DevControlMapper;
import com.yjyz.iot.device.dao.DeviceInfoMapper;
import com.yjyz.iot.device.entity.DevConsume;
import com.yjyz.iot.device.entity.DevConsumeKey;
import com.yjyz.iot.device.entity.DeviceInfo;
import com.yjyz.iot.dict.dao.DictChargeMapper;
import com.yjyz.iot.mq.producer.topic.TopicSender;
import com.yjyz.iot.security.dao.PowClientUserMapper;
import com.yjyz.iot.security.entity.PowClientUser;
import com.yjyz.iot.share.service.IClientService;

@Service("clientService")
public class ClientServiceImpl implements IClientService {

	@Resource
	PowClientUserMapper powClientUserDao;
	@Resource
	DictChargeMapper dictChargeDao;
	@Resource
	DevConsumeMapper devConsumeDao;
	@Autowired
	DeviceInfoMapper deviceInfoDao;
	@Resource
	DevAccountMapper devAccountDao;
	@Resource
	DevControlMapper devControlDao;
	@Autowired
	private TopicSender topicSender;
	@Value("${cfg.sys.device_ttl}")
	private long DEV_TTL;

	@Override
	public PowClientUser register(PowClientUser clientUser) throws Exception {
		PowClientUser user = this.powClientUserDao.selectByPrimaryKey(clientUser.getOpenid());
		if (user == null) {
			clientUser.setClientId(UUID.randomUUID().toString());
			this.powClientUserDao.insertSelective(clientUser);
		} else {
			clientUser.setClientId(user.getClientId());
			this.powClientUserDao.updateByPrimaryKeySelective(clientUser);
		}
		return clientUser;
	}

	@Override
	public PowClientUser getUserByOpenId(PowClientUser clientUser) throws Exception {
		return this.powClientUserDao.selectByPrimaryKey(clientUser.getOpenid());
	}

	@Override
	public PowClientUser getUserByOrderNo(String orderNo) throws Exception {
		DevConsume devConsume = this.devConsumeDao.selectByOrderNo(orderNo);
		return this.powClientUserDao.selectByClientId(devConsume.getClientId());
	}

	@Override
	public List<DevConsume> getConsumeHistory(String deviceId, String clientId) throws Exception {
		DevConsumeKey key = new DevConsumeKey();
		key.setClientId(clientId);
		key.setDeviceId(deviceId);
		return this.devConsumeDao.selectHistory(key);
	}

	@Override
	public int preBill(DevConsume devConsume) throws Exception {
		return this.devConsumeDao.insertSelective(devConsume);
	}

	@Override
	public int consume(String openId, DevConsume consume) throws Exception {
		DevConsume devConsume = this.devConsumeDao.selectByOrderNo(consume.getOrderNo());
		if (devConsume == null) {
			return ConstParm.ERR_NO_CONSUME;
		}

		PowClientUser powClientUser = this.powClientUserDao.selectByClientId(devConsume.getClientId());
		if (!powClientUser.getOpenid().equals(openId)) {
			return ConstParm.ERR_NO_CLIENT;
		}
		// 因目前筹码的确认是在设备端完的，所以修改成根据状态标识控制
		if (!devConsume.getStatus().equals("2")) {
			devConsume.setTotalFree(consume.getTotalFree());
			devConsume.setType(consume.getType());
			devConsume.setPrepayId(consume.getPrepayId());
			devConsume.setPayNo(consume.getPayNo());
			devConsume.setMchId(consume.getMchId());
			long curr_ts = System.currentTimeMillis();
			//devConsume.setCreateTime(new Date(curr_ts));
			devConsume.setBillBeing(new Date(curr_ts));
			devConsume.setBillExport(new Date(curr_ts + consume.getChips() * 60 * 1000));
			devConsume.setStatus(consume.getStatus());

			int ret = this.devConsumeDao.updateByPrimaryKeySelective(devConsume);
			if (ret != 1) {
				return ConstParm.ERR_INSERT;
			}

			Map<String, Object> cmap = new HashMap<String, Object>();
			cmap.put("CHIPS", devConsume.getChips());
			cmap.put("TYPE", "1"); // 0表示普通，1表示计费
			cmap.put("ORDERNO", devConsume.getOrderNo());
			cmap.put("STATUS", "1");
			JSONObject jObj = new JSONObject(cmap);

			topicSender.send("c2d." + devConsume.getDeviceId() + ".commands", jObj.toJSONString());

//			int tryCount = 0;
//			while (tryCount < 3) {
//				try {
//					Thread.currentThread();
//					Thread.sleep(1000);
//					devConsume = this.devConsumeDao.selectByOrderNo(consume.getOrderNo());
//					if (devConsume.getStatus().equals("2")) {
//						return ConstParm.SUCESS_CODE;
//					} else {
//						tryCount += 1;
//					}
//				} catch (InterruptedException e) {
//					e.printStackTrace();
//				}
//			}
			return ConstParm.SUCESS_CODE;
		} else if (devConsume.getStatus().equals("2")) {
			return ConstParm.SUCESS_CODE;
		}
		return ConstParm.ERR_SHARE_CLIENT_CONSUME;
	}

	public DevConsume getUnCosumeBill(String deviceMac) throws Exception {
		DeviceInfo devInfo = this.deviceInfoDao.selectByMAC(deviceMac);
		DevConsume dc = this.devConsumeDao.selectUnConOrder(devInfo.getDeviceId());

		return dc;
	}

}
