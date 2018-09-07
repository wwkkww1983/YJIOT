package com.yjyz.iot.security.action;

import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RestController;

import com.yjyz.iot.comm.ConstParm;
import com.yjyz.iot.comm.RetInfoDto;
import com.yjyz.iot.device.entity.DeviceInfo;
import com.yjyz.iot.security.dto.UserDevCommDto;
import com.yjyz.iot.security.entity.DeviceInfoQuery;
import com.yjyz.iot.security.entity.PowClientUser;
import com.yjyz.iot.security.entity.PowClientUserQuery;
import com.yjyz.iot.security.entity.PowUserDev;
import com.yjyz.iot.security.service.IClientPowerService;
import com.yjyz.iot.security.utils.BASE64Util;
import com.yjyz.iot.security.utils.ClientJwtToken;
import com.yjyz.iot.security.utils.ClientJwtUtil;
import com.yjyz.iot.security.utils.MD5Util;
import com.yjyz.iot.sms.SMSResult;

/**
 * @class :ClientPowerController
 * @TODO :设备用户权限管理
 * @author:Herolizhen
 * @date :2017年10月31日上午10:13:54
 */
@RestController
@RequestMapping("/clientPower")
public class ClientPowerController {
	@Autowired
	private IClientPowerService clientPowerService;
	@Autowired
	ClientJwtUtil jwtUtil;
	@Autowired
	MD5Util md5Util;
	@Autowired
	BASE64Util base64Util;

	private static Log log = LogFactory.getLog(ClientPowerController.class);

	/**
	 * @name:getVerCode
	 * @TODO: 用户注册登录获取验证码
	 * @date:2017年10月31日 上午10:14:20
	 * @param dto
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/getVerCode", method = RequestMethod.POST)
	public RetInfoDto getVerCode(@RequestBody UserDevCommDto dto) {

		RetInfoDto info = new RetInfoDto();
		try {
			if (!md5Util.MD5(dto.getAppId()).equals(dto.getKey())) {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_VERCODE);
				return info;
			}

			SMSResult rst = this.clientPowerService.getVerCode(dto.getAppId(), dto.getTel());
			if (rst.getCode().equals("0")) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.meta.put("message", rst.getMessage());
				info.data.put("verCode", rst.getVerCode());
			} else {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.meta.put("message", rst.getMessage());
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_VERCODE);
		}
		return info;
	}

	/**
	 * @name:userReg
	 * @TODO: 用户注册
	 * @date:2017年10月31日 上午10:15:17
	 * @param dto
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/userReg", method = RequestMethod.POST)
	public RetInfoDto userReg(@RequestBody UserDevCommDto dto) {
		RetInfoDto info = new RetInfoDto();
		try {
			PowClientUser user = new PowClientUser();
			user.setTel(dto.getTel());
			user.setPassword(dto.getPassword());
			user.setAppId(dto.getAppId());
			if (this.clientPowerService.userReg(user)) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_USERREG);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_USERREG);
		}
		return info;
	}

	/**
	 * @name:userLogin
	 * @TODO:用户登录
	 * @date:2017年10月31日 上午10:15:34
	 * @param dto
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/userLogin", method = RequestMethod.POST)
	public RetInfoDto userLogin(@RequestBody UserDevCommDto dto) {
		RetInfoDto info = new RetInfoDto();
		try {
			PowClientUser user = new PowClientUser();
			user.setTel(dto.getTel());
			user.setPassword(dto.getPassword());
			user.setAppId(dto.getAppId());
			user = this.clientPowerService.userLogin(user);
			if (user != null) {
				ClientJwtToken jwtToken = new ClientJwtToken(user.getClientId(), user.getAppId());
				String token = jwtUtil.createJWT(jwtToken);
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.data.put("token", token);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_LOGIN);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_LOGIN);
		}
		return info;
	}

	/**
	 * @name:getToken
	 * @TODO:重新获取token
	 * @date:2017年10月31日 上午10:15:48
	 * @param dto
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/getToken", method = RequestMethod.POST)
	public RetInfoDto getToken(@RequestBody UserDevCommDto dto) {
		RetInfoDto info = new RetInfoDto();

		try {
			if (!md5Util.MD5(dto.getAppId()).equals(dto.getKey())) {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_VERCODE_KEY);
				return info;
			}
			PowClientUser user = this.clientPowerService.getUserInfoByTel(dto.getTel());
			ClientJwtToken jwtToken = new ClientJwtToken(user.getClientId(), user.getAppId());
			String token = jwtUtil.createJWT(jwtToken);
			info.meta.put("code", ConstParm.SUCESS_CODE);
			info.data.put("token", token);
			return info;
		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "get Token fail.");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
	}

	/**
	 * @name:getUserInfo
	 * @TODO:获取用户信息
	 * @date:2017年10月31日 上午10:16:07
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/getUserInfo", method = RequestMethod.POST)
	public RetInfoDto getUserInfo(@RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {
			PowClientUser user = this.clientPowerService.getUserInfoByClientId(jwtToken.getUser_id());
			info.meta.put("code", ConstParm.SUCESS_CODE);
			info.data.put("user", user);
			return info;
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_GETUSERINFO);
			return info;
		}
	}

	/**
	 * @name:updateUserInfo
	 * @TODO:更新用户信息
	 * @date:2017年10月31日 上午10:16:52
	 * @param user
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/updateUserInfo", method = RequestMethod.POST)
	public RetInfoDto updateUserInfo(@RequestBody PowClientUser user, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {
			user.setClientId(jwtToken.getUser_id());
			user.setAppId(jwtToken.getApp_id());
			if (this.clientPowerService.updateUserInfo(user)) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_UPDATEUSER);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_UPDATEUSER);
		}
		return info;
	}

	/**
	 * @name:bindDevice
	 * @TODO:设备绑定
	 * @date:2017年10月31日 上午10:17:11
	 * @param dto
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/bindDevice", method = RequestMethod.POST)
	public RetInfoDto bindDevice(@RequestBody UserDevCommDto dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {

			PowUserDev userDev = new PowUserDev();
			userDev.setClientId(jwtToken.getUser_id());
			userDev.setDeviceId(dto.getDeviceId());
			userDev.setOwnType(dto.isOwnType());

			if (this.clientPowerService.bindDevice(userDev)) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_DEVBIND);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_DEVBIND);
		}
		return info;
	}

	/**
	 * @name:unBindDevice
	 * @TODO:设备解绑
	 * @date:2017年10月31日 上午10:17:25
	 * @param dto
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/unBindDevice", method = RequestMethod.POST)
	public RetInfoDto unBindDevice(@RequestBody UserDevCommDto dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {
			PowUserDev userDev = new PowUserDev();
			if (dto.getClientId() == null) {
				userDev.setClientId(jwtToken.getUser_id());
			} else {
				userDev.setClientId(dto.getClientId());
			}
			userDev.setDeviceId(dto.getDeviceId());
			if (this.clientPowerService.unBindDevice(userDev)) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_UNDEVBIND);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_UNDEVBIND);
		}
		return info;
	}

	/**
	 * @name:grantDeviceToUser
	 * @TODO:的通过电话号码授权用户
	 * @date:2017年10月31日 上午10:17:39
	 * @param dto
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/grantDeviceToUser", method = RequestMethod.POST)
	public RetInfoDto grantDeviceToUser(@RequestBody UserDevCommDto dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {
			if (this.clientPowerService.grantDeviceToUser(dto, jwtToken)) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_USERGRANT);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_USERGRANT);
		}
		return info;
	}

	/**
	 * @name:getUserDev
	 * @TODO:获取用户拥有设备
	 * @date:2017年10月31日 上午10:17:54
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/getUserDev", method = RequestMethod.POST)
	public RetInfoDto getUserDev(@RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}

		try {
			List<DeviceInfoQuery> devList = this.clientPowerService.getUserDevice(jwtToken.getUser_id());
			if (devList.size() > 0) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.data.put("devList", devList);
			} else {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			}

		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_GETUSERDEV);
		}
		return info;
	}

	/**
	 * @name:getDevUser
	 * @TODO:获取设备用户
	 * @date:2017年10月31日 上午10:18:07
	 * @param dto
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/getDevUser", method = RequestMethod.POST)
	public RetInfoDto getDevUser(@RequestBody UserDevCommDto dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		try {
			jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}

		try {
			List<PowClientUserQuery> userList = this.clientPowerService.getDeviceUser(dto.getDeviceId());
			if (userList.size() > 0) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.data.put("userList", userList);
			} else {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			}

		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_GETDEVUSER);
		}
		return info;
	}

	/**
	 * @name:getDevToken
	 * @TODO:获取设备TOKEN
	 * @date:2017年10月31日 上午10:18:20
	 * @param dto
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/getDevToken", method = RequestMethod.POST)
	public RetInfoDto getDevToken(@RequestBody UserDevCommDto dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		try {
			jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {
			String token = base64Util.getToken(dto.getDeviceId());
			info.meta.put("code", ConstParm.SUCESS_CODE);
			info.data.put("token", token);
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_GETDEVTOKEN);
		}
		return info;
	}

	/**
	 * @name:bindUser
	 * @TODO:通过验证码绑定设备
	 * @date:2017年10月31日 上午10:18:35
	 * @param dto
	 * @param Authorization
	 * @return RetInfoDto
	 */
	@Transactional
	@RequestMapping(value = "/bindUser", method = RequestMethod.POST)
	public RetInfoDto bindUser(@RequestBody UserDevCommDto dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}
		try {
			PowUserDev userDev = new PowUserDev();
			userDev.setClientId(jwtToken.getUser_id());
			userDev.setDeviceId(base64Util.getData(dto.getToken()));
			userDev.setOwnType(false);
			if (this.clientPowerService.bindDevice(userDev)) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_USERBIND);
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("code", ConstParm.ERR_SECURITY_CLIENT_USERBIND);
		}
		return info;
	}

	@Transactional
	@RequestMapping(value = "/updateDevInfo", method = RequestMethod.POST)
	public RetInfoDto updateDevInfo(@RequestBody DeviceInfo dto, @RequestHeader String Authorization) {
		RetInfoDto info = new RetInfoDto();
		try {
			jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}

		try {
			boolean isOk = this.clientPowerService.updateDevInfo(dto);
			if (isOk) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("message", "no dvice for this id.");
				info.meta.put("code", ConstParm.ERR_NO_DEVINFO);
			}

		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "updateDevInfoById fail.");
			info.meta.put("code", ConstParm.ERR_DEVICE_UPDATEDEVINFOBYID);
		}
		return info;
	}
}
