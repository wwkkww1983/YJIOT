package com.yjyz.iot.dict.action;

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
import com.yjyz.iot.dict.entity.DictCharge;
import com.yjyz.iot.dict.service.IDictService;
import com.yjyz.iot.security.utils.ClientJwtToken;
import com.yjyz.iot.security.utils.ClientJwtUtil;

/**
 * 
 * @class :ChargeController
 * @TODO :
 * @author:Herolizhen
 * @date :2017年11月7日下午5:35:35
 */
@RestController
@RequestMapping("/dict")
public class ChargeController {
	private static Log log = LogFactory.getLog(ChargeController.class);
	@Autowired
	private IDictService dictService;
	@Autowired
	ClientJwtUtil jwtUtil;

	@Transactional
	@RequestMapping(value = "/addCharge", method = RequestMethod.POST)
	public RetInfoDto addCharge(@RequestBody DictCharge dto, @RequestHeader String Authorization) {

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
			dto.setAppId(jwtToken.getApp_id());
			DictCharge dictCharge = this.dictService.addCharge(dto);
			if (dictCharge != null) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.data.put("dictCharge", dictCharge);
			} else {
				info.meta.put("code", ConstParm.ERR_INSERT);
				info.meta.put("message", "addCharge fail.");
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "addCharge fail.");
			info.meta.put("code", ConstParm.ERR_INSERT);
		}
		return info;
	}

	@Transactional
	@RequestMapping(value = "/delCharge", method = RequestMethod.POST)
	public RetInfoDto delCharge(@RequestBody DictCharge dto, @RequestHeader String Authorization) {

		RetInfoDto info = new RetInfoDto();
		try {
			jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}

		try {
			boolean isOk = this.dictService.delCharge(dto);
			if (isOk) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_DELETE);
				info.meta.put("message", "delCharge fail.");
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "delCharge fail.");
			info.meta.put("code", ConstParm.ERR_DELETE);
		}
		return info;
	}

	@Transactional
	@RequestMapping(value = "/updCharge", method = RequestMethod.POST)
	public RetInfoDto updCharge(@RequestBody DictCharge dto, @RequestHeader String Authorization) {

		RetInfoDto info = new RetInfoDto();
		try {
			jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}

		try {
			boolean isOk = this.dictService.updCharge(dto);
			if (isOk) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
			} else {
				info.meta.put("code", ConstParm.ERR_UPDATE);
				info.meta.put("message", "uptCharge fail.");
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "uptCharge fail.");
			info.meta.put("code", ConstParm.ERR_UPDATE);
		}
		return info;
	}

	@Transactional
	@RequestMapping(value = "/selChargeByType", method = RequestMethod.POST)
	public RetInfoDto selChargeByType(@RequestBody DictCharge dto, @RequestHeader String Authorization) {

		RetInfoDto info = new RetInfoDto();
		ClientJwtToken jwtToken;
		try {
			jwtToken = 	jwtUtil.parseToken(Authorization);
		} catch (Exception e) {
			info.meta.put("message", "access token is wrong!");
			info.meta.put("code", ConstParm.ERR_CODE_JWT);
			return info;
		}

		try {
			dto.setAppId(jwtToken.getApp_id());
			List<DictCharge> chargeList = this.dictService.selChargeByType(dto);
			if (chargeList.size() > 0) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.data.put("chargeList", chargeList);
			} else {
				//info.meta.put("code", ConstParm.ERR_NO_CHARGE);
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.meta.put("message", "selChargeByType no data.");
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "selChargeByType fail.");
			info.meta.put("code", ConstParm.ERR_SELECT);
		}
		return info;
	}

	@Transactional
	@RequestMapping(value = "/selChargeAll", method = RequestMethod.POST)
	public RetInfoDto selChargeAll(@RequestHeader String Authorization) {

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
			List<DictCharge> chargeList = this.dictService.selChargeAll(jwtToken.getApp_id());
			if (chargeList.size() > 0) {
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.data.put("chargeList", chargeList);
			} else {
				//info.meta.put("code", ConstParm.ERR_NO_CHARGE);
				info.meta.put("code", ConstParm.SUCESS_CODE);
				info.meta.put("message", "selChargeByType no data.");
			}
		} catch (Exception e) {
			log.error(e);
			info.meta.put("message", "selChargeByType fail.");
			info.meta.put("code", ConstParm.ERR_SELECT);
		}
		return info;
	}

}
