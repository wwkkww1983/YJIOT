package com.yjyz.iot.security.entity;

public class PowResource {
	private String resId;

	private String resName;

	private String resDetail;

	private String appId;

	private String resType;

	public String getResId() {
		return resId;
	}

	public void setResId(String resId) {
		this.resId = resId == null ? null : resId.trim();
	}

	public String getResName() {
		return resName;
	}

	public void setResName(String resName) {
		this.resName = resName == null ? null : resName.trim();
	}

	public String getResDetail() {
		return resDetail;
	}

	public void setResDetail(String resDetail) {
		this.resDetail = resDetail == null ? null : resDetail.trim();
	}

	public String getAppId() {
		return appId;
	}

	public void setAppId(String appId) {
		this.appId = appId == null ? null : appId.trim();
	}

	public String getResType() {
		return resType;
	}

	public void setResType(String resType) {
		this.resType = resType == null ? null : resType.trim();
	}
}