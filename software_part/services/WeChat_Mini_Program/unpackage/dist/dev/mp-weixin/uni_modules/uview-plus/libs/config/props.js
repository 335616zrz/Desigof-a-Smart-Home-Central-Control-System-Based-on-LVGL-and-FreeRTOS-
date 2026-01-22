"use strict";
const common_vendor = require("../../../../common/vendor.js");
const uni_modules_uviewPlus_libs_config_config = require("./config.js");
const uni_modules_uviewPlus_libs_config_zIndex = require("./zIndex.js");
const uni_modules_uviewPlus_libs_config_color = require("./color.js");
const uni_modules_uviewPlus_libs_function_http = require("../function/http.js");
const uni_modules_uviewPlus_libs_function_index = require("../function/index.js");
const uni_modules_uviewPlus_components_uActionSheet_actionSheet = require("../../components/u-action-sheet/actionSheet.js");
const uni_modules_uviewPlus_components_uAlbum_album = require("../../components/u-album/album.js");
const uni_modules_uviewPlus_components_uAlert_alert = require("../../components/u-alert/alert.js");
const uni_modules_uviewPlus_components_uAvatar_avatar = require("../../components/u-avatar/avatar.js");
const uni_modules_uviewPlus_components_uAvatarGroup_avatarGroup = require("../../components/u-avatar-group/avatarGroup.js");
const uni_modules_uviewPlus_components_uBackTop_backtop = require("../../components/u-back-top/backtop.js");
const uni_modules_uviewPlus_components_uBadge_badge = require("../../components/u-badge/badge.js");
const uni_modules_uviewPlus_components_uButton_button = require("../../components/u-button/button.js");
const uni_modules_uviewPlus_components_uCalendar_calendar = require("../../components/u-calendar/calendar.js");
const uni_modules_uviewPlus_components_uCarKeyboard_carKeyboard = require("../../components/u-car-keyboard/carKeyboard.js");
const uni_modules_uviewPlus_components_uCard_card = require("../../components/u-card/card.js");
const uni_modules_uviewPlus_components_uCell_cell = require("../../components/u-cell/cell.js");
const uni_modules_uviewPlus_components_uCellGroup_cellGroup = require("../../components/u-cell-group/cellGroup.js");
const uni_modules_uviewPlus_components_uCheckbox_checkbox = require("../../components/u-checkbox/checkbox.js");
const uni_modules_uviewPlus_components_uCheckboxGroup_checkboxGroup = require("../../components/u-checkbox-group/checkboxGroup.js");
const uni_modules_uviewPlus_components_uCircleProgress_circleProgress = require("../../components/u-circle-progress/circleProgress.js");
const uni_modules_uviewPlus_components_uCode_code = require("../../components/u-code/code.js");
const uni_modules_uviewPlus_components_uCodeInput_codeInput = require("../../components/u-code-input/codeInput.js");
const uni_modules_uviewPlus_components_uCol_col = require("../../components/u-col/col.js");
const uni_modules_uviewPlus_components_uCollapse_collapse = require("../../components/u-collapse/collapse.js");
const uni_modules_uviewPlus_components_uCollapseItem_collapseItem = require("../../components/u-collapse-item/collapseItem.js");
const uni_modules_uviewPlus_components_uColumnNotice_columnNotice = require("../../components/u-column-notice/columnNotice.js");
const uni_modules_uviewPlus_components_uCountDown_countDown = require("../../components/u-count-down/countDown.js");
const uni_modules_uviewPlus_components_uCountTo_countTo = require("../../components/u-count-to/countTo.js");
const uni_modules_uviewPlus_components_uDatetimePicker_datetimePicker = require("../../components/u-datetime-picker/datetimePicker.js");
const uni_modules_uviewPlus_components_uDivider_divider = require("../../components/u-divider/divider.js");
const uni_modules_uviewPlus_components_uEmpty_empty = require("../../components/u-empty/empty.js");
const uni_modules_uviewPlus_components_uForm_form = require("../../components/u-form/form.js");
const uni_modules_uviewPlus_components_uFormItem_formItem = require("../../components/u-form-item/formItem.js");
const uni_modules_uviewPlus_components_uGap_gap = require("../../components/u-gap/gap.js");
const uni_modules_uviewPlus_components_uGrid_grid = require("../../components/u-grid/grid.js");
const uni_modules_uviewPlus_components_uGridItem_gridItem = require("../../components/u-grid-item/gridItem.js");
const uni_modules_uviewPlus_components_uIcon_icon = require("../../components/u-icon/icon.js");
const uni_modules_uviewPlus_components_uImage_image = require("../../components/u-image/image.js");
const uni_modules_uviewPlus_components_uIndexAnchor_indexAnchor = require("../../components/u-index-anchor/indexAnchor.js");
const uni_modules_uviewPlus_components_uIndexList_indexList = require("../../components/u-index-list/indexList.js");
const uni_modules_uviewPlus_components_uInput_input = require("../../components/u-input/input.js");
const uni_modules_uviewPlus_components_uKeyboard_keyboard = require("../../components/u-keyboard/keyboard.js");
const uni_modules_uviewPlus_components_uLine_line = require("../../components/u-line/line.js");
const uni_modules_uviewPlus_components_uLineProgress_lineProgress = require("../../components/u-line-progress/lineProgress.js");
const uni_modules_uviewPlus_components_uLink_link = require("../../components/u-link/link.js");
const uni_modules_uviewPlus_components_uList_list = require("../../components/u-list/list.js");
const uni_modules_uviewPlus_components_uListItem_listItem = require("../../components/u-list-item/listItem.js");
const uni_modules_uviewPlus_components_uLoadingIcon_loadingIcon = require("../../components/u-loading-icon/loadingIcon.js");
const uni_modules_uviewPlus_components_uLoadingPage_loadingPage = require("../../components/u-loading-page/loadingPage.js");
const uni_modules_uviewPlus_components_uLoadmore_loadmore = require("../../components/u-loadmore/loadmore.js");
const uni_modules_uviewPlus_components_uModal_modal = require("../../components/u-modal/modal.js");
const uni_modules_uviewPlus_components_uNavbar_navbar = require("../../components/u-navbar/navbar.js");
const uni_modules_uviewPlus_components_uNoNetwork_noNetwork = require("../../components/u-no-network/noNetwork.js");
const uni_modules_uviewPlus_components_uNoticeBar_noticeBar = require("../../components/u-notice-bar/noticeBar.js");
const uni_modules_uviewPlus_components_uNotify_notify = require("../../components/u-notify/notify.js");
const uni_modules_uviewPlus_components_uNumberBox_numberBox = require("../../components/u-number-box/numberBox.js");
const uni_modules_uviewPlus_components_uNumberKeyboard_numberKeyboard = require("../../components/u-number-keyboard/numberKeyboard.js");
const uni_modules_uviewPlus_components_uOverlay_overlay = require("../../components/u-overlay/overlay.js");
const uni_modules_uviewPlus_components_uParse_parse = require("../../components/u-parse/parse.js");
const uni_modules_uviewPlus_components_uPicker_picker = require("../../components/u-picker/picker.js");
const uni_modules_uviewPlus_components_uPopup_popup = require("../../components/u-popup/popup.js");
const uni_modules_uviewPlus_components_uRadio_radio = require("../../components/u-radio/radio.js");
const uni_modules_uviewPlus_components_uRadioGroup_radioGroup = require("../../components/u-radio-group/radioGroup.js");
const uni_modules_uviewPlus_components_uRate_rate = require("../../components/u-rate/rate.js");
const uni_modules_uviewPlus_components_uReadMore_readMore = require("../../components/u-read-more/readMore.js");
const uni_modules_uviewPlus_components_uRow_row = require("../../components/u-row/row.js");
const uni_modules_uviewPlus_components_uRowNotice_rowNotice = require("../../components/u-row-notice/rowNotice.js");
const uni_modules_uviewPlus_components_uScrollList_scrollList = require("../../components/u-scroll-list/scrollList.js");
const uni_modules_uviewPlus_components_uSearch_search = require("../../components/u-search/search.js");
const uni_modules_uviewPlus_components_uSection_section = require("../../components/u-section/section.js");
const uni_modules_uviewPlus_components_uSkeleton_skeleton = require("../../components/u-skeleton/skeleton.js");
const uni_modules_uviewPlus_components_uSlider_slider = require("../../components/u-slider/slider.js");
const uni_modules_uviewPlus_components_uStatusBar_statusBar = require("../../components/u-status-bar/statusBar.js");
const uni_modules_uviewPlus_components_uSteps_steps = require("../../components/u-steps/steps.js");
const uni_modules_uviewPlus_components_uStepsItem_stepsItem = require("../../components/u-steps-item/stepsItem.js");
const uni_modules_uviewPlus_components_uSticky_sticky = require("../../components/u-sticky/sticky.js");
const uni_modules_uviewPlus_components_uSubsection_subsection = require("../../components/u-subsection/subsection.js");
const uni_modules_uviewPlus_components_uSwipeAction_swipeAction = require("../../components/u-swipe-action/swipeAction.js");
const uni_modules_uviewPlus_components_uSwipeActionItem_swipeActionItem = require("../../components/u-swipe-action-item/swipeActionItem.js");
const uni_modules_uviewPlus_components_uSwiper_swiper = require("../../components/u-swiper/swiper.js");
const uni_modules_uviewPlus_components_uSwiperIndicator_swipterIndicator = require("../../components/u-swiper-indicator/swipterIndicator.js");
const uni_modules_uviewPlus_components_uSwitch_switch = require("../../components/u-switch/switch.js");
const uni_modules_uviewPlus_components_uTabbar_tabbar = require("../../components/u-tabbar/tabbar.js");
const uni_modules_uviewPlus_components_uTabbarItem_tabbarItem = require("../../components/u-tabbar-item/tabbarItem.js");
const uni_modules_uviewPlus_components_uTabs_tabs = require("../../components/u-tabs/tabs.js");
const uni_modules_uviewPlus_components_uTag_tag = require("../../components/u-tag/tag.js");
const uni_modules_uviewPlus_components_uText_text = require("../../components/u-text/text.js");
const uni_modules_uviewPlus_components_uTextarea_textarea = require("../../components/u-textarea/textarea.js");
const uni_modules_uviewPlus_components_uToast_toast = require("../../components/u-toast/toast.js");
const uni_modules_uviewPlus_components_uToolbar_toolbar = require("../../components/u-toolbar/toolbar.js");
const uni_modules_uviewPlus_components_uTooltip_tooltip = require("../../components/u-tooltip/tooltip.js");
const uni_modules_uviewPlus_components_uTransition_transition = require("../../components/u-transition/transition.js");
const uni_modules_uviewPlus_components_uUpload_upload = require("../../components/u-upload/upload.js");
const props = {
  ...uni_modules_uviewPlus_components_uActionSheet_actionSheet.ActionSheet,
  ...uni_modules_uviewPlus_components_uAlbum_album.Album,
  ...uni_modules_uviewPlus_components_uAlert_alert.Alert,
  ...uni_modules_uviewPlus_components_uAvatar_avatar.Avatar,
  ...uni_modules_uviewPlus_components_uAvatarGroup_avatarGroup.AvatarGroup,
  ...uni_modules_uviewPlus_components_uBackTop_backtop.Backtop,
  ...uni_modules_uviewPlus_components_uBadge_badge.Badge,
  ...uni_modules_uviewPlus_components_uButton_button.Button,
  ...uni_modules_uviewPlus_components_uCalendar_calendar.Calendar,
  ...uni_modules_uviewPlus_components_uCarKeyboard_carKeyboard.CarKeyboard,
  ...uni_modules_uviewPlus_components_uCard_card.Card,
  ...uni_modules_uviewPlus_components_uCell_cell.Cell,
  ...uni_modules_uviewPlus_components_uCellGroup_cellGroup.CellGroup,
  ...uni_modules_uviewPlus_components_uCheckbox_checkbox.Checkbox,
  ...uni_modules_uviewPlus_components_uCheckboxGroup_checkboxGroup.CheckboxGroup,
  ...uni_modules_uviewPlus_components_uCircleProgress_circleProgress.CircleProgress,
  ...uni_modules_uviewPlus_components_uCode_code.Code,
  ...uni_modules_uviewPlus_components_uCodeInput_codeInput.CodeInput,
  ...uni_modules_uviewPlus_components_uCol_col.Col,
  ...uni_modules_uviewPlus_components_uCollapse_collapse.Collapse,
  ...uni_modules_uviewPlus_components_uCollapseItem_collapseItem.CollapseItem,
  ...uni_modules_uviewPlus_components_uColumnNotice_columnNotice.ColumnNotice,
  ...uni_modules_uviewPlus_components_uCountDown_countDown.CountDown,
  ...uni_modules_uviewPlus_components_uCountTo_countTo.CountTo,
  ...uni_modules_uviewPlus_components_uDatetimePicker_datetimePicker.DatetimePicker,
  ...uni_modules_uviewPlus_components_uDivider_divider.Divider,
  ...uni_modules_uviewPlus_components_uEmpty_empty.Empty,
  ...uni_modules_uviewPlus_components_uForm_form.Form,
  ...uni_modules_uviewPlus_components_uFormItem_formItem.FormItem,
  ...uni_modules_uviewPlus_components_uGap_gap.Gap,
  ...uni_modules_uviewPlus_components_uGrid_grid.Grid,
  ...uni_modules_uviewPlus_components_uGridItem_gridItem.GridItem,
  ...uni_modules_uviewPlus_components_uIcon_icon.Icon,
  ...uni_modules_uviewPlus_components_uImage_image.Image,
  ...uni_modules_uviewPlus_components_uIndexAnchor_indexAnchor.IndexAnchor,
  ...uni_modules_uviewPlus_components_uIndexList_indexList.IndexList,
  ...uni_modules_uviewPlus_components_uInput_input.Input,
  ...uni_modules_uviewPlus_components_uKeyboard_keyboard.Keyboard,
  ...uni_modules_uviewPlus_components_uLine_line.Line,
  ...uni_modules_uviewPlus_components_uLineProgress_lineProgress.LineProgress,
  ...uni_modules_uviewPlus_components_uLink_link.Link,
  ...uni_modules_uviewPlus_components_uList_list.List,
  ...uni_modules_uviewPlus_components_uListItem_listItem.ListItem,
  ...uni_modules_uviewPlus_components_uLoadingIcon_loadingIcon.LoadingIcon,
  ...uni_modules_uviewPlus_components_uLoadingPage_loadingPage.LoadingPage,
  ...uni_modules_uviewPlus_components_uLoadmore_loadmore.Loadmore,
  ...uni_modules_uviewPlus_components_uModal_modal.Modal,
  ...uni_modules_uviewPlus_components_uNavbar_navbar.Navbar,
  ...uni_modules_uviewPlus_components_uNoNetwork_noNetwork.NoNetwork,
  ...uni_modules_uviewPlus_components_uNoticeBar_noticeBar.NoticeBar,
  ...uni_modules_uviewPlus_components_uNotify_notify.Notify,
  ...uni_modules_uviewPlus_components_uNumberBox_numberBox.NumberBox,
  ...uni_modules_uviewPlus_components_uNumberKeyboard_numberKeyboard.NumberKeyboard,
  ...uni_modules_uviewPlus_components_uOverlay_overlay.Overlay,
  ...uni_modules_uviewPlus_components_uParse_parse.Parse,
  ...uni_modules_uviewPlus_components_uPicker_picker.Picker,
  ...uni_modules_uviewPlus_components_uPopup_popup.Popup,
  ...uni_modules_uviewPlus_components_uRadio_radio.Radio,
  ...uni_modules_uviewPlus_components_uRadioGroup_radioGroup.RadioGroup,
  ...uni_modules_uviewPlus_components_uRate_rate.Rate,
  ...uni_modules_uviewPlus_components_uReadMore_readMore.ReadMore,
  ...uni_modules_uviewPlus_components_uRow_row.Row,
  ...uni_modules_uviewPlus_components_uRowNotice_rowNotice.RowNotice,
  ...uni_modules_uviewPlus_components_uScrollList_scrollList.ScrollList,
  ...uni_modules_uviewPlus_components_uSearch_search.Search,
  ...uni_modules_uviewPlus_components_uSection_section.Section,
  ...uni_modules_uviewPlus_components_uSkeleton_skeleton.Skeleton,
  ...uni_modules_uviewPlus_components_uSlider_slider.Slider,
  ...uni_modules_uviewPlus_components_uStatusBar_statusBar.StatusBar,
  ...uni_modules_uviewPlus_components_uSteps_steps.Steps,
  ...uni_modules_uviewPlus_components_uStepsItem_stepsItem.StepsItem,
  ...uni_modules_uviewPlus_components_uSticky_sticky.Sticky,
  ...uni_modules_uviewPlus_components_uSubsection_subsection.Subsection,
  ...uni_modules_uviewPlus_components_uSwipeAction_swipeAction.SwipeAction,
  ...uni_modules_uviewPlus_components_uSwipeActionItem_swipeActionItem.SwipeActionItem,
  ...uni_modules_uviewPlus_components_uSwiper_swiper.Swiper,
  ...uni_modules_uviewPlus_components_uSwiperIndicator_swipterIndicator.SwipterIndicator,
  ...uni_modules_uviewPlus_components_uSwitch_switch.Switch,
  ...uni_modules_uviewPlus_components_uTabbar_tabbar.Tabbar,
  ...uni_modules_uviewPlus_components_uTabbarItem_tabbarItem.TabbarItem,
  ...uni_modules_uviewPlus_components_uTabs_tabs.Tabs,
  ...uni_modules_uviewPlus_components_uTag_tag.Tag,
  ...uni_modules_uviewPlus_components_uText_text.Text,
  ...uni_modules_uviewPlus_components_uTextarea_textarea.Textarea,
  ...uni_modules_uviewPlus_components_uToast_toast.Toast,
  ...uni_modules_uviewPlus_components_uToolbar_toolbar.Toolbar,
  ...uni_modules_uviewPlus_components_uTooltip_tooltip.Tooltip,
  ...uni_modules_uviewPlus_components_uTransition_transition.Transition,
  ...uni_modules_uviewPlus_components_uUpload_upload.Upload
};
function setConfig(configs) {
  uni_modules_uviewPlus_libs_function_index.shallowMerge(uni_modules_uviewPlus_libs_config_config.config, configs.config || {});
  uni_modules_uviewPlus_libs_function_index.shallowMerge(props, configs.props || {});
  uni_modules_uviewPlus_libs_function_index.shallowMerge(uni_modules_uviewPlus_libs_config_color.color, configs.color || {});
  uni_modules_uviewPlus_libs_function_index.shallowMerge(uni_modules_uviewPlus_libs_config_zIndex.zIndex, configs.zIndex || {});
}
if (common_vendor.index && common_vendor.index.upuiParams) {
  common_vendor.index.__f__("log", "at uni_modules/uview-plus/libs/config/props.js:206", "setting uview-plus");
  let temp = common_vendor.index.upuiParams();
  if (temp.httpIns) {
    temp.httpIns(uni_modules_uviewPlus_libs_function_http.http);
  }
  if (temp.options) {
    setConfig(temp.options);
  }
}
exports.props = props;
//# sourceMappingURL=../../../../../.sourcemap/mp-weixin/uni_modules/uview-plus/libs/config/props.js.map
